/* $Id: fcb.c,v 1.8 2001/07/28 07:05:56 hbirr Exp $
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

  return  rcFCB;
}

VOID
vfatDestroyFCB(PVFATFCB  pFCB)
{
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
  if (pFCB->RefCount <= 0 && !vfatFCBIsDirectory (pVCB, pFCB))
  {
    RemoveEntryList (&pFCB->FcbListEntry);
    CcRosReleaseFileCache (NULL, pFCB->RFCB.Bcb);
    vfatDestroyFCB (pFCB);
  }
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
vfatFCBInitializeCache (PVCB  vcb, PVFATFCB  fcb)
{
  NTSTATUS  status;
  PFILE_OBJECT  fileObject;
  ULONG  bytesPerCluster;
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
  fcb->pDevExt = vcb;

  bytesPerCluster = vcb->Boot->SectorsPerCluster * BLOCKSIZE;
  fileCacheQuantum = (bytesPerCluster >= PAGESIZE) ? 
      bytesPerCluster : PAGESIZE;
  status = CcRosInitializeFileCache (fileObject, 
                                     &fcb->RFCB.Bcb,
                                     fileCacheQuantum);
  if (!NT_SUCCESS (status))
  {
    DbgPrint ("CcRosInitializeFileCache failed\n");
    KeBugCheck (0);
  }
  ObDereferenceObject (fileObject);
  fcb->isCacheInitialized = TRUE;

  return  status;
}

NTSTATUS
vfatRequestAndValidateRegion(PDEVICE_EXTENSION pDeviceExt,
			     PVFATFCB pFCB,
			     ULONG  pOffset,
			     PVOID * pBuffer,
			     PCACHE_SEGMENT * pCacheSegment,
			     BOOL pExtend)
{
  NTSTATUS  status;
  BOOLEAN  valid;
  BOOLEAN  isRoot;
  ULONG  currentCluster;
  ULONG  i;

  status = CcRosRequestCacheSegment(pFCB->RFCB.Bcb,
                                    pOffset,
                                    pBuffer,
                                    &valid,
                                    pCacheSegment);
  if (!NT_SUCCESS (status))
  {
    return  status;
  }

  isRoot = vfatFCBIsRoot (pFCB);
  if (!valid)
  {
    currentCluster = vfatDirEntryGetFirstCluster (pDeviceExt, &pFCB->entry);
    status = OffsetToCluster (pDeviceExt, 
                              vfatDirEntryGetFirstCluster (pDeviceExt, &pFCB->entry), 
                              pOffset, 
                              &currentCluster,
                              pExtend);
    if (!NT_SUCCESS (status))
    {
      return  status;
    }

    if (PAGESIZE > pDeviceExt->BytesPerCluster)
    {
      for (i = 0; i < (PAGESIZE / pDeviceExt->BytesPerCluster); i++)
      {
        status = VfatRawReadCluster (pDeviceExt, 
                                     vfatDirEntryGetFirstCluster (pDeviceExt, &pFCB->entry),
                                     ((PCHAR)*pBuffer) + 
                                     (i * pDeviceExt->BytesPerCluster),
                                     currentCluster);
        if (!NT_SUCCESS (status))
        {
          CcRosReleaseCacheSegment(pFCB->RFCB.Bcb, *pCacheSegment, FALSE);
          return  status;
        }
        status = NextCluster (pDeviceExt, 
                              vfatDirEntryGetFirstCluster (pDeviceExt, &pFCB->entry), 
                              &currentCluster, 
                              pExtend);
        if (!NT_SUCCESS (status))
        {
          CcRosReleaseCacheSegment(pFCB->RFCB.Bcb, *pCacheSegment, FALSE);
          return  status;
        }
        if ((currentCluster) == 0xFFFFFFFF)
        {
          break;
        }
      }
    }
    else
    {
      status = VfatRawReadCluster (pDeviceExt, 
                                   vfatDirEntryGetFirstCluster (pDeviceExt, &pFCB->entry),
                                   *pBuffer,
                                   currentCluster);
      if (!NT_SUCCESS (status))
      {
        CcRosReleaseCacheSegment(pFCB->RFCB.Bcb, *pCacheSegment, FALSE);
        return  status;
      }
    }
  }

  return  STATUS_SUCCESS;
}

NTSTATUS
vfatReleaseRegion (PDEVICE_EXTENSION  pDeviceExt,
                   PVFATFCB  pFCB,
                   PCACHE_SEGMENT  pCacheSegment)
{
  return  CcRosReleaseCacheSegment (pFCB->RFCB.Bcb, pCacheSegment, TRUE);
}

PVFATFCB
vfatMakeRootFCB(PDEVICE_EXTENSION  pVCB)
{
  PVFATFCB  FCB;

  FCB = vfatNewFCB(L"\\");
  memset(FCB->entry.Filename, ' ', 11);
  FCB->entry.FileSize = pVCB->rootDirectorySectors * BLOCKSIZE;
  FCB->entry.Attrib = FILE_ATTRIBUTE_DIRECTORY;
  if (pVCB->FatType == FAT32)
    {
      FCB->entry.FirstCluster = ((struct _BootSector32*)(pVCB->Boot))->RootCluster & 0xffff;
      FCB->entry.FirstClusterHigh = ((struct _BootSector32*)(pVCB->Boot))->RootCluster >> 16;
    }
  else
    {
      FCB->entry.FirstCluster = 1;
    }
  FCB->RefCount = 1;

  vfatFCBInitializeCache(pVCB, FCB);
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
			PVFATFCB * fileFCB)
{
  PVFATFCB  rcFCB;
  WCHAR  pathName [MAX_PATH];

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

  vfatFCBInitializeCache (vcb, rcFCB);
  vfatAddFCBToTable (vcb, rcFCB);
  vfatGrabFCB (vcb, rcFCB);
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

  if (!fcb->isCacheInitialized)
  {
    ULONG  bytesPerCluster;
    ULONG  fileCacheQuantum;

    bytesPerCluster = vcb->Boot->SectorsPerCluster * BLOCKSIZE;
    fileCacheQuantum = (bytesPerCluster >= PAGESIZE) ? bytesPerCluster : 
        PAGESIZE;
    status = CcRosInitializeFileCache (fileObject, 
                                       &fcb->RFCB.Bcb,
                                       fileCacheQuantum);
    if (!NT_SUCCESS (status))
    {
      DbgPrint ("CcRosInitializeFileCache failed\n");
      KeBugCheck (0);
    }
    fcb->isCacheInitialized = TRUE;
  }

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




