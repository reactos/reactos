/* $Id: fcb.c,v 1.23 2002/12/15 17:01:51 chorns Exp $
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

ULONG vfatNameHash(ULONG hash, PWCHAR name)
{
  WCHAR c;
  while(c = *name++)
  {
    c = towlower(c);
    hash = (hash + (c << 4) + (c >> 4)) * 11;
  }
  return hash;
}

PVFATFCB
vfatNewFCB(PWCHAR pFileName)
{
  PVFATFCB  rcFCB;

  rcFCB = ExAllocateFromNPagedLookasideList(&VfatGlobalData->FcbLookasideList);
  memset (rcFCB, 0, sizeof (VFATFCB));
  if (pFileName)
  {
    wcscpy (rcFCB->PathName, pFileName);
    rcFCB->ObjectName = wcsrchr(rcFCB->PathName, L'\\');
    if (rcFCB->ObjectName == NULL)
    {
      rcFCB->ObjectName = rcFCB->PathName;
    }
    rcFCB->Hash.Hash = vfatNameHash(0, rcFCB->PathName);
    DPRINT("%08x (%03x) '%S'\n", rcFCB->Hash.Hash, rcFCB->Hash.Hash % FCB_HASH_TABLE_SIZE, pFileName);
  }
  rcFCB->Hash.self = rcFCB;
  rcFCB->ShortHash.self = rcFCB;
  ExInitializeResourceLite(&rcFCB->PagingIoResource);
  ExInitializeResourceLite(&rcFCB->MainResource);
  return  rcFCB;
}

VOID 
vfatDestroyCCB(PVFATCCB pCcb)
{
  if (pCcb->DirectorySearchPattern)
  {
     ExFreePool(pCcb->DirectorySearchPattern);
  }
  ExFreeToNPagedLookasideList(&VfatGlobalData->CcbLookasideList, pCcb);
}

VOID
vfatDestroyFCB(PVFATFCB  pFCB)
{
  ExDeleteResourceLite(&pFCB->PagingIoResource);
  ExDeleteResourceLite(&pFCB->MainResource);
  if ((pFCB->Flags & FCB_IS_PAGE_FILE) && pFCB->FatChainSize)
	ExFreePool(pFCB->FatChain);
  ExFreeToNPagedLookasideList(&VfatGlobalData->FcbLookasideList, pFCB);
}

BOOL
vfatFCBIsDirectory(PVFATFCB FCB)
{
  return  FCB->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY;
}

BOOL
vfatFCBIsRoot(PVFATFCB FCB)
{
    return  FCB->PathName[0] == L'\\' && FCB->PathName[1] == 0 ? TRUE : FALSE;
}

VOID
vfatReleaseFCB(PDEVICE_EXTENSION  pVCB,  PVFATFCB  pFCB)
{
  KIRQL  oldIrql;
  HASHENTRY* entry;
  ULONG Index;
  ULONG ShortIndex;

  DPRINT ("releasing FCB at %x: %S, refCount:%d\n",
          pFCB,
          pFCB->PathName,
          pFCB->RefCount);

  Index = pFCB->Hash.Hash % FCB_HASH_TABLE_SIZE;
  ShortIndex = pFCB->ShortHash.Hash % FCB_HASH_TABLE_SIZE;
  KeAcquireSpinLock (&pVCB->FcbListLock, &oldIrql);
  pFCB->RefCount--;
  if (pFCB->RefCount <= 0 && (!vfatFCBIsDirectory (pFCB) || pFCB->Flags & FCB_DELETE_PENDING))
  {
    RemoveEntryList (&pFCB->FcbListEntry);  
    if (pFCB->Hash.Hash != pFCB->ShortHash.Hash)
    {
      entry = pVCB->FcbHashTable[ShortIndex];
      if (entry->self == pFCB)
      {
         pVCB->FcbHashTable[ShortIndex] = entry->next;
      }
      else
      {
         while (entry->next->self != pFCB)
	 {
            entry = entry->next;
	 }
         entry->next = pFCB->ShortHash.next;
      }
    }
    entry = pVCB->FcbHashTable[Index];
    if (entry->self == pFCB)
    {
      pVCB->FcbHashTable[Index] = entry->next;
    }
    else
    {
      while (entry->next->self != pFCB)
      {
        entry = entry->next;
      }
      entry->next = pFCB->Hash.next;
    }
    KeReleaseSpinLock (&pVCB->FcbListLock, oldIrql);
    if (vfatFCBIsDirectory(pFCB))
    {
      /* Uninitialize file cache if initialized for this file object. */
      if (pFCB->RFCB.Bcb != NULL)
        {
          CcRosReleaseFileCache(pFCB->FileObject, pFCB->RFCB.Bcb);
        }
      vfatDestroyCCB(pFCB->FileObject->FsContext2);
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
  ULONG Index;
  ULONG ShortIndex;

  Index = pFCB->Hash.Hash % FCB_HASH_TABLE_SIZE;
  ShortIndex = pFCB->ShortHash.Hash % FCB_HASH_TABLE_SIZE;
  KeAcquireSpinLock (&pVCB->FcbListLock, &oldIrql);
  pFCB->pDevExt = pVCB;
  InsertTailList (&pVCB->FcbListHead, &pFCB->FcbListEntry);
   
  pFCB->Hash.next = pVCB->FcbHashTable[Index];
  pVCB->FcbHashTable[Index] = &pFCB->Hash;
  if (pFCB->Hash.Hash != pFCB->ShortHash.Hash)
  {
     pFCB->ShortHash.next = pVCB->FcbHashTable[ShortIndex];
     pVCB->FcbHashTable[ShortIndex] = &pFCB->ShortHash;
  }
  KeReleaseSpinLock (&pVCB->FcbListLock, oldIrql);
}

PVFATFCB
vfatGrabFCBFromTable(PDEVICE_EXTENSION  pVCB, PWSTR  pFileName)
{
  KIRQL  oldIrql;
  PVFATFCB  rcFCB;
  PLIST_ENTRY  current_entry;
  ULONG Hash;
  PWCHAR ObjectName = NULL;
  ULONG len;
  ULONG index;
  ULONG currentindex;
  
  HASHENTRY* entry; 

  Hash = vfatNameHash(0, pFileName);

  KeAcquireSpinLock (&pVCB->FcbListLock, &oldIrql);
  entry = pVCB->FcbHashTable[Hash % FCB_HASH_TABLE_SIZE];

  while (entry)
  {
     if (entry->Hash == Hash)
     {
        rcFCB = entry->self;
	if (rcFCB->Hash.Hash == Hash)
	{
	   /* compare the long name */
	   if (!_wcsicmp(pFileName, rcFCB->PathName))
	   {
              rcFCB->RefCount++;
              KeReleaseSpinLock (&pVCB->FcbListLock, oldIrql);
              return rcFCB;
	   }
	}
	else
	{
	   len = rcFCB->ObjectName - rcFCB->PathName + 1;
	   if (ObjectName == NULL)
	   {
	      ObjectName = wcsrchr(pFileName, L'\\');
              if (ObjectName == NULL)
	      {
                ObjectName = pFileName;
	      }
	      else
	      {
		ObjectName++;
	      }
	   }

	   /* compare the short name and the directory */
	   if (!_wcsicmp(ObjectName, rcFCB->ShortName) && !_wcsnicmp(pFileName, rcFCB->PathName, len))
	   {
              rcFCB->RefCount++;
              KeReleaseSpinLock (&pVCB->FcbListLock, oldIrql);
              return rcFCB;
	   }
	}
     }
     entry = entry->next;
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

  newCCB = ExAllocateFromNPagedLookasideList(&VfatGlobalData->CcbLookasideList);
  if (newCCB == NULL)
  {
    return  STATUS_INSUFFICIENT_RESOURCES;
  }
  memset (newCCB, 0, sizeof (VFATCCB));

  fileObject->Flags |= FO_FCB_IS_VALID | FO_DIRECT_CACHE_PAGING_READ;
  fileObject->SectionObjectPointers = &fcb->SectionObjectPointers;
  fileObject->FsContext = (PVOID) &fcb->RFCB;
  fileObject->FsContext2 = newCCB;
  newCCB->pFcb = fcb;
  newCCB->PtrFileObject = fileObject;
  fcb->FileObject = fileObject;
  fcb->pDevExt = vcb;


  fileCacheQuantum = (vcb->FatInfo.BytesPerCluster >= PAGE_SIZE) ?
      vcb->FatInfo.BytesPerCluster : PAGE_SIZE;

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
  FCB->ShortName[0] = L'\\';
  FCB->ShortName[1] = 0;
  FCB->ShortHash.Hash = FCB->Hash.Hash;
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
  FCB->RefCount = 2;
  FCB->dirIndex = 0;
  FCB->RFCB.FileSize.QuadPart = Size;
  FCB->RFCB.ValidDataLength.QuadPart = Size;
  FCB->RFCB.AllocationSize.QuadPart = Size;

  vfatFCBInitializeCacheFromVolume(pVCB, FCB);
  vfatAddFCBToTable(pVCB, FCB);

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
			ULONG startIndex,
			ULONG dirIndex,
			PVFATFCB* fileFCB)
{
  PVFATFCB  rcFCB;
  WCHAR  pathName [MAX_PATH];
  WCHAR  entryName [14];
  ULONG Size;
  ULONG hash;
  
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
  hash = vfatNameHash(0, pathName);
  vfatGetDirEntryName (dirEntry, entryName);
  if (longName [0] != 0)
  {
    wcscat (pathName, longName);
  }
  else
  {
    wcscat (pathName, entryName);
  }
  rcFCB = vfatNewFCB (pathName);
  memcpy (&rcFCB->entry, dirEntry, sizeof (FAT_DIR_ENTRY));
  wcscpy(rcFCB->ShortName, entryName);
  rcFCB->ShortHash.Hash = vfatNameHash(hash, entryName);
  
  if (vfatFCBIsDirectory(rcFCB))
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
  rcFCB->startIndex = startIndex;
  rcFCB->RFCB.FileSize.QuadPart = Size;
  rcFCB->RFCB.ValidDataLength.QuadPart = Size;
  rcFCB->RFCB.AllocationSize.QuadPart = ROUND_UP(Size, vcb->FatInfo.BytesPerCluster);
  rcFCB->RefCount++;
  if (vfatFCBIsDirectory(rcFCB))
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

  newCCB = ExAllocateFromNPagedLookasideList(&VfatGlobalData->CcbLookasideList);
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
  ULONG  directoryIndex;
  ULONG startIndex;
  NTSTATUS  status;
  WCHAR  defaultFileName [2];
  WCHAR  currentLongName [256];
  FAT_DIR_ENTRY  currentDirEntry;
  WCHAR  currentEntryName [256];
  PVOID Context = NULL;
  PVOID Page;

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
  while (TRUE)
  {
    status = vfatGetNextDirEntry(&Context,
	                         &Page, 
	                         pDirectoryFCB,
				 &directoryIndex,
				 currentLongName,
				 &currentDirEntry,
				 &startIndex);
    if (status == STATUS_NO_MORE_ENTRIES)
    {
      return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    DPRINT ("  Index:%d  longName:%S\n",
            directoryIndex,
            currentLongName);

    if (!vfatIsDirEntryVolume(&currentDirEntry))
    {
      if (currentLongName [0] != L'\0' && wstrcmpjoki (currentLongName, pFileToFind))
      {
        DPRINT ("Match found, %S\n", currentLongName);
        status = vfatMakeFCBFromDirEntry (pDeviceExt,
                                          pDirectoryFCB,
                                          currentLongName,
                                          &currentDirEntry,
					  startIndex, 
                                          directoryIndex,
                                          pFoundFCB);
	CcUnpinData(Context);
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
					    startIndex,
                                            directoryIndex,
                                            pFoundFCB);
          CcUnpinData(Context);
          return  status;
        }
      }
    }
    directoryIndex++;
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

  currentElement = wcsrchr(pFileName, L'\\');
  wcsncpy(pathName, pFileName, currentElement - pFileName);
  currentElement++;

  FCB = vfatGrabFCBFromTable(pVCB, pathName);
  if (FCB == NULL)
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
    if (!vfatFCBIsDirectory (FCB))
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




