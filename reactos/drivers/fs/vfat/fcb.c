/* $Id: fcb.c,v 1.3 2001/05/10 06:30:23 rex Exp $
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

PVFATFCB  vfatNewFCB (PWCHAR pFileName)
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

void  vfatDestroyFCB (PVFATFCB  pFCB)
{
  ExFreePool (pFCB);
}

void  vfatGrabFCB (PDEVICE_EXTENSION  pVCB, PVFATFCB  pFCB)
{
  KIRQL  oldIrql;

  KeAcquireSpinLock (&pVCB->FcbListLock, &oldIrql);
  pFCB->RefCount++;
  KeReleaseSpinLock (&pVCB->FcbListLock, oldIrql);
}

void  vfatReleaseFCB (PDEVICE_EXTENSION  pVCB,  PVFATFCB  pFCB)
{
  KIRQL  oldIrql;

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

void  vfatAddFCBToTable (PDEVICE_EXTENSION  pVCB,  PVFATFCB  pFCB)
{
  KIRQL  oldIrql;

  KeAcquireSpinLock (&pVCB->FcbListLock, &oldIrql);
  pFCB->pDevExt = pVCB;
  InsertTailList (&pVCB->FcbListHead, &pFCB->FcbListEntry);
  KeReleaseSpinLock (&pVCB->FcbListLock, oldIrql);
}

PVFATFCB  
vfatGrabFCBFromTable (PDEVICE_EXTENSION  pVCB, PWSTR  pFileName)
{
  KIRQL  oldIrql;
  PVFATFCB  rcFCB;
  PLIST_ENTRY  current_entry;

  CHECKPOINT;
  KeAcquireSpinLock (&pVCB->FcbListLock, &oldIrql);
  CHECKPOINT;
  current_entry = pVCB->FcbListHead.Flink;
  while (current_entry != &pVCB->FcbListHead)
  {
    rcFCB = CONTAINING_RECORD (current_entry, VFATFCB, FcbListEntry);

    DPRINT ("Next FCB in list at %x\n", rcFCB);
    DPRINT ("  PathName:%S\n", rcFCB->PathName);

    if (wstrcmpi (pFileName, rcFCB->PathName))
    {
      rcFCB->RefCount++;
      KeReleaseSpinLock (&pVCB->FcbListLock, oldIrql);
      return  rcFCB;
    }
    current_entry = current_entry->Flink;
  }
  CHECKPOINT;
  KeReleaseSpinLock (&pVCB->FcbListLock, oldIrql);

  return  NULL;
}

PVFATFCB  
vfatMakeRootFCB (PDEVICE_EXTENSION  pVCB)
{
  NTSTATUS  status;
  PVFATFCB  FCB;
  PFILE_OBJECT  fileObject;
  ULONG  bytesPerCluster;
  ULONG  fileCacheQuantum;

  FCB = vfatNewFCB (L"\\");
  memset (FCB->entry.Filename, ' ', 11);
  FCB->entry.FileSize = pVCB->rootDirectorySectors * BLOCKSIZE;
  FCB->entry.Attrib = FILE_ATTRIBUTE_DIRECTORY;
  if (pVCB->FatType == FAT32)
  {
    FCB->entry.FirstCluster = 2;
  }
  else
  {
    FCB->entry.FirstCluster = 1;
  }
  FCB->RefCount = 1;
  fileObject = IoCreateStreamFileObject (NULL, pVCB->StorageDevice);

  bytesPerCluster = pVCB->Boot->SectorsPerCluster * BLOCKSIZE;
  fileCacheQuantum = (bytesPerCluster >= PAGESIZE) ? 
      bytesPerCluster : PAGESIZE;
  status = CcRosInitializeFileCache (fileObject, 
                                     &FCB->RFCB.Bcb,
                                     fileCacheQuantum);
  if (!NT_SUCCESS (status))
  {
    DbgPrint ("CcRosInitializeFileCache failed\n");
    KeBugCheck (0);
  }
  ObDereferenceObject (fileObject);
  vfatAddFCBToTable (pVCB, FCB);

  return  FCB;
}

PVFATFCB  
vfatOpenRootFCB (PDEVICE_EXTENSION  pVCB)
{
  PVFATFCB  FCB;

  FCB = vfatGrabFCBFromTable (pVCB, L"\\");
  if (FCB != NULL)
  {
    return  FCB;
  }
  FCB = vfatMakeRootFCB (pVCB);
  
  return  FCB;
}

BOOL
vfatFCBIsDirectory (PDEVICE_EXTENSION pVCB, PVFATFCB FCB)
{
  return  FCB->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY;
}

PVFATFCB  
vfatDirFindFile (PDEVICE_EXTENSION  pVCB, 
                 PVFATFCB  parentFCB, 
                 const PWSTR  elementName)
{
  UNIMPLEMENTED;
}

NTSTATUS
vfatGetFCBForFile (PDEVICE_EXTENSION  pVCB, 
                   PVFATFCB  *pParentFCB, 
                   PVFATFCB  *pFCB, 
                   const PWSTR  pFileName)
{
  WCHAR  pathName [MAX_PATH];
  WCHAR  elementName [MAX_PATH];
  PWCHAR  currentElement;
  PVFATFCB  FCB;
  PVFATFCB  parentFCB;
  
  //  Trivial case, open of the root directory on volume
  if (pFileName [0] == L'\0' || wcscmp (pFileName, L"\\") == 0)
  {
    currentElement = pFileName;
    //FIXME: grab/create root RCB and return it
    FCB = vfatGrabFCBFromTable (pVCB, L"\\");
    if (FCB == NULL)
    {
      FCB = vfatMakeRootFCB (pVCB);
      *pFCB = FCB;
      *pParentFCB = NULL;

      return  (FCB != NULL) ? STATUS_SUCCESS : STATUS_OBJECT_PATH_NOT_FOUND;
    }
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

    currentElement = vfatGetNextPathElement (currentElement);

    //  descend to next directory level
    if (parentFCB)
    {
      vfatReleaseFCB (pVCB, parentFCB);
      parentFCB = 0;
    }
    //  fail if element in FCB is not a directory
    if (!vfatFCBIsDirectory (pVCB, FCB))
    {
      vfatReleaseFCB (pVCB, FCB);
      FCB = 0;
      return  STATUS_OBJECT_PATH_NOT_FOUND;
    }
    parentFCB = FCB;

    //  Extract next directory level into dirName
    vfatWSubString (pathName, 
                    pFileName, 
                    vfatGetNextPathElement (currentElement) - pFileName);

    FCB = vfatGrabFCBFromTable (pVCB, pathName);
    if (FCB == NULL)
    {
      vfatWSubString (elementName, 
                      currentElement, 
                      vfatGetNextPathElement (currentElement) - currentElement);
      FCB = vfatDirFindFile (pVCB, parentFCB, elementName);
      if (FCB == NULL)
      {
        *pParentFCB = parentFCB;
        *pFCB = NULL;
        if (vfatGetNextPathElement (currentElement) == 0)
        {
          return  STATUS_OBJECT_NAME_NOT_FOUND;
        }
        else
        {
          return  STATUS_OBJECT_PATH_NOT_FOUND;
        }
      }

      // FIXME: check security on directory element and fail if access denied
    }
  }

  *pParentFCB = parentFCB;
  *pFCB = FCB;

  return  STATUS_SUCCESS;
}




