/* $Id: fcb.c,v 1.20 2002/09/08 10:22:12 chorns Exp $
 *
 *
 * FILE:             fcb.c
 * PURPOSE:          Routines to manipulate FCBs.
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *                   Rex Jolliff (rex@lvcablemodem.com)
 */

/*  -------------------------------------------------------  INCLUDES  */

#include <ddk/ntddk.h>
#include <wchar.h>
#include <limits.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/*  --------------------------------------------------------  DEFINES  */

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#define TAG_FCB TAG('V', 'F', 'C', 'B')

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))

/*  --------------------------------------------------------  PUBLICS  */

PVFATFCB
vfatNewFCB(PWCHAR pFileName)
{
  PVFATFCB  rcFCB;

  rcFCB = ExAllocatePoolWithTag (NonPagedPool, sizeof (VFATFCB), TAG_FCB);
  memset (rcFCB, 0, sizeof (VFATFCB));
  if (pFileName)
  {
    wcscpy (rcFCB->PathName, pFileName);
    if (wcsrchr (rcFCB->PathName, '\\') != 0)
    {
      rcFCB->ObjectName = wcsrchr (rcFCB->PathName, '\\');
    }
    else
    {
      rcFCB->ObjectName = rcFCB->PathName;
    }
  }
  ExInitializeResourceLite(&rcFCB->PagingIoResource);
  ExInitializeResourceLite(&rcFCB->MainResource);
  return  rcFCB;
}

VOID
vfatDestroyFCB(PVFATFCB  pFCB)
{
  ExDeleteResourceLite(&pFCB->PagingIoResource);
  ExDeleteResourceLite(&pFCB->MainResource);
  if ((pFCB->Flags & FCB_IS_PAGE_FILE) && pFCB->FatChainSize)
	ExFreePool(pFCB->FatChain);
  ExFreePool (pFCB);
}

BOOL
vfatFCBIsDirectory(PDEVICE_EXTENSION pVCB, PVFATFCB FCB)
{
  return  FCB->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY;
}

BOOL
vfatFCBIsRoot(PVFATFCB FCB)
{
  return  wcscmp (FCB->PathName, L"\\") == 0;
}

VOID
vfatGrabFCB(PDEVICE_EXTENSION  pVCB, PVFATFCB  pFCB)
{
  KIRQL  oldIrql;

  DPRINT ("grabbing FCB at %x: %S, refCount:%d\n",
          pFCB,
          pFCB->PathName,
          pFCB->RefCount);

  KeAcquireSpinLock (&pVCB->FcbListLock, &oldIrql);
  pFCB->RefCount++;
  KeReleaseSpinLock (&pVCB->FcbListLock, oldIrql);
}

VOID
vfatReleaseFCB(PDEVICE_EXTENSION  pVCB,  PVFATFCB  pFCB)
{
  KIRQL  oldIrql;

  DPRINT ("releasing FCB at %x: %S, refCount:%d\n",
          pFCB,
          pFCB->PathName,
          pFCB->RefCount);

  KeAcquireSpinLock (&pVCB->FcbListLock, &oldIrql);
  pFCB->RefCount--;
  if (pFCB->RefCount <= 0 && (!vfatFCBIsDirectory (pVCB, pFCB) || pFCB->Flags & FCB_DELETE_PENDING))
  {
    RemoveEntryList (&pFCB->FcbListEntry);    
    KeReleaseSpinLock (&pVCB->FcbListLock, oldIrql);
    if (vfatFCBIsDirectory(pVCB, pFCB))
    {
      CcRosReleaseFileCache(pFCB->FileObject, pFCB->RFCB.Bcb);
      ExFreePool(pFCB->FileObject->FsContext2);
      pFCB->FileObject->FsContext2 = NULL;
      ObDereferenceObject(pFCB->FileObject);
    }
    vfatDestroyFCB (pFCB);
  }
  else
    KeReleaseSpinLock (&pVCB->FcbListLock, oldIrql);
}

VOID
vfatAddFCBToTable(PDEVICE_EXTENSION  pVCB,  PVFATFCB  pFCB)
{
  KIRQL  oldIrql;

  KeAcquireSpinLock (&pVCB->FcbListLock, &oldIrql);
  pFCB->pDevExt = pVCB;
  InsertTailList (&pVCB->FcbListHead, &pFCB->FcbListEntry);
  KeReleaseSpinLock (&pVCB->FcbListLock, oldIrql);
}

PVFATFCB
vfatGrabFCBFromTable(PDEVICE_EXTENSION  pVCB, PWSTR  pFileName)
{
  KIRQL  oldIrql;
  PVFATFCB  rcFCB;
  PLIST_ENTRY  current_entry;

  KeAcquireSpinLock (&pVCB->FcbListLock, &oldIrql);
  current_entry = pVCB->FcbListHead.Flink;
  while (current_entry != &pVCB->FcbListHead)
  {
    rcFCB = CONTAINING_RECORD (current_entry, VFATFCB, FcbListEntry);

    if (wstrcmpi (pFileName, rcFCB->PathName))
    {
      rcFCB->RefCount++;
      KeReleaseSpinLock (&pVCB->FcbListLock, oldIrql);
      return  rcFCB;
    }

    //FIXME: need to compare against short name in FCB here

    current_entry = current_entry->Flink;
  }
  KeReleaseSpinLock (&pVCB->FcbListLock, oldIrql);

  return  NULL;
}

NTSTATUS
vfatFCBInitializeCacheFromVolume (PVCB  vcb, PVFATFCB  fcb)
{
  NTSTATUS  status;
  PFILE_OBJECT  fileObject;
  ULONG  fileCacheQuantum;
  PVFATCCB  newCCB;

  fileObject = IoCreateStreamFileObject (NULL, vcb->StorageDevice);

  newCCB = ExAllocatePoolWithTag (NonPagedPool, sizeof (VFATCCB), TAG_CCB);
  if (newCCB == NULL)
  {
    return  STATUS_INSUFFICIENT_RESOURCES;
  }
  memset (newCCB, 0, sizeof (VFATCCB));

  fileObject->Flags = fileObject->Flags | FO_FCB_IS_VALID |
      FO_DIRECT_CACHE_PAGING_READ;
  fileObject->SectionObjectPointers = &fcb->SectionObjectPointers;
  fileObject->FsContext = (PVOID) &fcb->RFCB;
  fileObject->FsContext2 = newCCB;
  newCCB->pFcb = fcb;
  newCCB->PtrFileObject = fileObject;
  fcb->FileObject = fileObject;
  fcb->pDevExt = vcb;


  fileCacheQuantum = (vcb->FatInfo.BytesPerCluster >= PAGESIZE) ?
      vcb->FatInfo.BytesPerCluster : PAGESIZE;

  status = CcRosInitializeFileCache (fileObject,
                                     &fcb->RFCB.Bcb,
                                     fileCacheQuantum);
  if (!NT_SUCCESS (status))
  {
    DbgPrint ("CcRosInitializeFileCache failed\n");
    KeBugCheck (0);
  }

  fcb->Flags |= FCB_CACHE_INITIALIZED;

  return  status;
}

PVFATFCB
vfatMakeRootFCB(PDEVICE_EXTENSION  pVCB)
{
  PVFATFCB  FCB;
  ULONG FirstCluster, CurrentCluster, Size = 0;
  NTSTATUS Status = STATUS_SUCCESS;

  FCB = vfatNewFCB(L"\\");
  memset(FCB->entry.Filename, ' ', 11);
  FCB->entry.FileSize = pVCB->FatInfo.rootDirectorySectors * pVCB->FatInfo.BytesPerSector;
  FCB->entry.Attrib = FILE_ATTRIBUTE_DIRECTORY;
  if (pVCB->FatInfo.FatType == FAT32)
  {
    CurrentCluster = FirstCluster = pVCB->FatInfo.RootCluster;
    FCB->entry.FirstCluster = FirstCluster & 0xffff;
    FCB->entry.FirstClusterHigh = FirstCluster >> 16;
    CurrentCluster = FirstCluster;

    while (CurrentCluster != 0xffffffff && NT_SUCCESS(Status))
    {
      Size += pVCB->FatInfo.BytesPerCluster;
      Status = NextCluster (pVCB, NULL, FirstCluster, &CurrentCluster, FALSE);
    }
  }
  else
  {
    FCB->entry.FirstCluster = 1;
    Size = pVCB->FatInfo.rootDirectorySectors * pVCB->FatInfo.BytesPerSector;
  }
  FCB->RefCount = 1;
  FCB->dirIndex = 0;
  FCB->RFCB.FileSize.QuadPart = Size;
  FCB->RFCB.ValidDataLength.QuadPart = Size;
  FCB->RFCB.AllocationSize.QuadPart = Size;

  vfatFCBInitializeCacheFromVolume(pVCB, FCB);
  vfatAddFCBToTable(pVCB, FCB);
  vfatGrabFCB(pVCB, FCB);

  return(FCB);
}

PVFATFCB
vfatOpenRootFCB(PDEVICE_EXTENSION  pVCB)
{
  PVFATFCB  FCB;

  FCB = vfatGrabFCBFromTable (pVCB, L"\\");
  if (FCB == NULL)
  {
    FCB = vfatMakeRootFCB (pVCB);
  }

  return  FCB;
}

NTSTATUS
vfatMakeFCBFromDirEntry(PVCB  vcb,
			PVFATFCB  directoryFCB,
			PWSTR  longName,
			PFAT_DIR_ENTRY  dirEntry,
			ULONG dirIndex,
			PVFATFCB* fileFCB)
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
      Size = vcb->FatInfo.rootDirectorySectors * vcb->FatInfo.BytesPerSector;
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
  rcFCB->RefCount++;
  if (vfatFCBIsDirectory(vcb, rcFCB))
    {
      vfatFCBInitializeCacheFromVolume(vcb, rcFCB);
    }
  vfatAddFCBToTable (vcb, rcFCB);
  *fileFCB = rcFCB;

  return  STATUS_SUCCESS;
}

NTSTATUS
vfatAttachFCBToFileObject (PDEVICE_EXTENSION  vcb,
                           PVFATFCB  fcb,
                           PFILE_OBJECT  fileObject)
{
  NTSTATUS  status;
  PVFATCCB  newCCB;

  newCCB = ExAllocatePoolWithTag (NonPagedPool, sizeof (VFATCCB), TAG_CCB);
  if (newCCB == NULL)
  {
    return  STATUS_INSUFFICIENT_RESOURCES;
  }
  memset (newCCB, 0, sizeof (VFATCCB));

  fileObject->Flags = fileObject->Flags | FO_FCB_IS_VALID |
      FO_DIRECT_CACHE_PAGING_READ;
  fileObject->SectionObjectPointers = &fcb->SectionObjectPointers;
  fileObject->FsContext = (PVOID) &fcb->RFCB;
  fileObject->FsContext2 = newCCB;
  newCCB->pFcb = fcb;
  newCCB->PtrFileObject = fileObject;
  fcb->pDevExt = vcb;
  DPRINT ("file open: fcb:%x file size: %d\n", fcb, fcb->entry.FileSize);

  return  STATUS_SUCCESS;
}

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

NTSTATUS
vfatGetFCBForFile (PDEVICE_EXTENSION  pVCB,
                   PVFATFCB  *pParentFCB,
                   PVFATFCB  *pFCB,
                   const PWSTR  pFileName)
{
  NTSTATUS  status;
  WCHAR  pathName [MAX_PATH];
  WCHAR  elementName [MAX_PATH];
  PWCHAR  currentElement;
  PVFATFCB  FCB;
  PVFATFCB  parentFCB;

  DPRINT ("vfatGetFCBForFile (%x,%x,%x,%S)\n",
          pVCB,
          pParentFCB,
          pFCB,
          pFileName);

  //  Trivial case, open of the root directory on volume
  if (pFileName [0] == L'\0' || wcscmp (pFileName, L"\\") == 0)
  {
    DPRINT ("returning root FCB\n");

    FCB = vfatOpenRootFCB (pVCB);
    *pFCB = FCB;
    *pParentFCB = NULL;

    return  (FCB != NULL) ? STATUS_SUCCESS : STATUS_OBJECT_PATH_NOT_FOUND;
  }
  else
  {
    currentElement = pFileName + 1;
    wcscpy (pathName, L"\\");
    FCB = vfatOpenRootFCB (pVCB);
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
}




