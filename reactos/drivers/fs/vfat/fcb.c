/* $Id$
 *
 *
 * FILE:             drivers/fs/vfat/fcb.c
 * PURPOSE:          Routines to manipulate FCBs.
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *                   Rex Jolliff (rex@lvcablemodem.com)
 *                   Hartmut Birr
 *                   Herve Poussineau (reactos@poussine.freesurf.fr)
 */

/*  -------------------------------------------------------  INCLUDES  */

#include <ddk/ntddk.h>
#include <wchar.h>
#include <limits.h>
#include <rosrtl/string.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/*  --------------------------------------------------------  DEFINES  */

#define TAG_FCB TAG('V', 'F', 'C', 'B')

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))

/*  --------------------------------------------------------  PUBLICS  */

ULONG vfatNameHash(ULONG hash, PUNICODE_STRING NameU)
{
  PWCHAR last; 
  PWCHAR curr;
  register WCHAR c;

  ASSERT(NameU->Buffer[0] != L'.');
  curr = NameU->Buffer;
  last = NameU->Buffer + NameU->Length / sizeof(WCHAR);

  while(curr < last)
    {
      c = towlower(*curr++);
      hash = (hash + (c << 4) + (c >> 4)) * 11;
    }
  return hash;
}

VOID
vfatSplitPathName(PUNICODE_STRING PathNameU, PUNICODE_STRING DirNameU, PUNICODE_STRING FileNameU)
{
  PWCHAR pName;
  USHORT Length = 0;
  pName = PathNameU->Buffer + PathNameU->Length / sizeof(WCHAR) - 1;
  while (*pName != L'\\' && pName >= PathNameU->Buffer)
    {
      pName--;
      Length++;
    }
  ASSERT(*pName == L'\\' || pName < PathNameU->Buffer);
  if (FileNameU)
    {
      FileNameU->Buffer = pName + 1;
      FileNameU->Length = FileNameU->MaximumLength = Length * sizeof(WCHAR);
    }
  if (DirNameU)
    {
      DirNameU->Buffer = PathNameU->Buffer;
      DirNameU->Length = (pName + 1 - PathNameU->Buffer) * sizeof(WCHAR);
      DirNameU->MaximumLength = DirNameU->Length;
    }
}

VOID
vfatInitFcb(PVFATFCB Fcb, PUNICODE_STRING NameU)
{
  ULONG PathNameBufferLength;
  
  if (NameU)
    PathNameBufferLength = NameU->Length + sizeof(WCHAR);
  else
    PathNameBufferLength = 0;
  
  Fcb->PathNameBuffer = ExAllocatePool(NonPagedPool, PathNameBufferLength);
  if (!Fcb->PathNameBuffer)
  {
    /* FIXME: what to do if no more memory? */
    DPRINT1("Unable to initialize FCB for filename '%wZ'\n", NameU);
    KEBUGCHECKEX(0, (ULONG_PTR)Fcb, (ULONG_PTR)NameU, 0, 0);
  }
  
  Fcb->PathNameU.Length = 0;
  Fcb->PathNameU.Buffer = Fcb->PathNameBuffer;
  Fcb->PathNameU.MaximumLength = PathNameBufferLength;
  Fcb->ShortNameU.Length = 0;
  Fcb->ShortNameU.Buffer = Fcb->ShortNameBuffer;
  Fcb->ShortNameU.MaximumLength = sizeof(Fcb->ShortNameBuffer);
  Fcb->DirNameU.Buffer = Fcb->PathNameU.Buffer;
  if (NameU && NameU->Length)
    {
      RtlCopyUnicodeString(&Fcb->PathNameU, NameU);
      vfatSplitPathName(&Fcb->PathNameU, &Fcb->DirNameU, &Fcb->LongNameU);
    }
  else
    {
      Fcb->DirNameU.Buffer = Fcb->LongNameU.Buffer = NULL;
      Fcb->DirNameU.MaximumLength = Fcb->DirNameU.Length = 0;
      Fcb->LongNameU.MaximumLength = Fcb->LongNameU.Length = 0; 
    }
  RtlZeroMemory(&Fcb->FCBShareAccess, sizeof(SHARE_ACCESS));
  Fcb->OpenHandleCount = 0;
}  

PVFATFCB
vfatNewFCB(PDEVICE_EXTENSION  pVCB, PUNICODE_STRING pFileNameU)
{
  PVFATFCB  rcFCB;

  DPRINT("'%wZ'\n", pFileNameU);

  rcFCB = ExAllocateFromNPagedLookasideList(&VfatGlobalData->FcbLookasideList);
  if (rcFCB == NULL)
    {
      return NULL;
    }
  RtlZeroMemory(rcFCB, sizeof(VFATFCB));
  vfatInitFcb(rcFCB, pFileNameU);
  if (pVCB->Flags & VCB_IS_FATX)
  {
    rcFCB->Flags |= FCB_IS_FATX_ENTRY;
    rcFCB->Attributes = &rcFCB->entry.FatX.Attrib;
  }
  else
    rcFCB->Attributes = &rcFCB->entry.Fat.Attrib;
  rcFCB->Hash.Hash = vfatNameHash(0, &rcFCB->PathNameU);
  rcFCB->Hash.self = rcFCB;
  rcFCB->ShortHash.self = rcFCB;
  ExInitializeResourceLite(&rcFCB->PagingIoResource);
  ExInitializeResourceLite(&rcFCB->MainResource);
  FsRtlInitializeFileLock(&rcFCB->FileLock, NULL, NULL); 
  ExInitializeFastMutex(&rcFCB->LastMutex);
  return  rcFCB;
}

VOID 
vfatDestroyCCB(PVFATCCB pCcb)
{
  if (pCcb->SearchPattern.Buffer)
    {
      ExFreePool(pCcb->SearchPattern.Buffer);
    }
  ExFreeToNPagedLookasideList(&VfatGlobalData->CcbLookasideList, pCcb);
}

VOID
vfatDestroyFCB(PVFATFCB pFCB)
{
  FsRtlUninitializeFileLock(&pFCB->FileLock); 
  ExFreePool(pFCB->PathNameBuffer);
  ExDeleteResourceLite(&pFCB->PagingIoResource);
  ExDeleteResourceLite(&pFCB->MainResource);
  ExFreeToNPagedLookasideList(&VfatGlobalData->FcbLookasideList, pFCB);
}

BOOLEAN
vfatFCBIsDirectory(PVFATFCB FCB)
{
  return  *FCB->Attributes & FILE_ATTRIBUTE_DIRECTORY;
}

BOOLEAN
vfatFCBIsRoot(PVFATFCB FCB)
{
  return  FCB->PathNameU.Length == sizeof(WCHAR) && FCB->PathNameU.Buffer[0] == L'\\' ? TRUE : FALSE;
}

VOID
vfatReleaseFCB(PDEVICE_EXTENSION  pVCB,  PVFATFCB  pFCB)
{
  HASHENTRY* entry;
  ULONG Index;
  ULONG ShortIndex;
  PVFATFCB tmpFcb;

  DPRINT ("releasing FCB at %x: %wZ, refCount:%d\n",
          pFCB,
          &pFCB->PathNameU,
          pFCB->RefCount);

  while (pFCB)
  {
     Index = pFCB->Hash.Hash % FCB_HASH_TABLE_SIZE;
     ShortIndex = pFCB->ShortHash.Hash % FCB_HASH_TABLE_SIZE;
     pFCB->RefCount--;
     if (pFCB->RefCount <= 0 && (!vfatFCBIsDirectory (pFCB) || pFCB->Flags & FCB_DELETE_PENDING))
     {
        tmpFcb = pFCB->parentFcb;
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
        if (vfatFCBIsDirectory(pFCB))
        {
           /* Uninitialize file cache if initialized for this file object. */
           if (pFCB->FileObject->SectionObjectPointer->SharedCacheMap)
	   {
              CcRosReleaseFileCache(pFCB->FileObject);
	   }
           vfatDestroyCCB(pFCB->FileObject->FsContext2);
           pFCB->FileObject->FsContext2 = NULL;
	   pFCB->FileObject->FsContext = NULL;
           ObDereferenceObject(pFCB->FileObject);
        }
        vfatDestroyFCB (pFCB);
     }
     else
     {
	tmpFcb = NULL;
     }
     pFCB = tmpFcb;
  }
}

VOID
vfatAddFCBToTable(PDEVICE_EXTENSION  pVCB,  PVFATFCB  pFCB)
{
  ULONG Index;
  ULONG ShortIndex;

  Index = pFCB->Hash.Hash % FCB_HASH_TABLE_SIZE;
  ShortIndex = pFCB->ShortHash.Hash % FCB_HASH_TABLE_SIZE;

  InsertTailList (&pVCB->FcbListHead, &pFCB->FcbListEntry);
   
  pFCB->Hash.next = pVCB->FcbHashTable[Index];
  pVCB->FcbHashTable[Index] = &pFCB->Hash;
  if (pFCB->Hash.Hash != pFCB->ShortHash.Hash)
  {
     pFCB->ShortHash.next = pVCB->FcbHashTable[ShortIndex];
     pVCB->FcbHashTable[ShortIndex] = &pFCB->ShortHash;
  }
  if (pFCB->parentFcb)
  {
     pFCB->parentFcb->RefCount++;
  }
}

PVFATFCB
vfatGrabFCBFromTable(PDEVICE_EXTENSION  pVCB, PUNICODE_STRING  PathNameU)
{
  PVFATFCB  rcFCB;
  ULONG Hash;
  UNICODE_STRING DirNameU;
  UNICODE_STRING FileNameU;
  PUNICODE_STRING FcbNameU;
  
  HASHENTRY* entry; 

  DPRINT("'%wZ'\n", PathNameU);
  
  Hash = vfatNameHash(0, PathNameU);

  entry = pVCB->FcbHashTable[Hash % FCB_HASH_TABLE_SIZE];
  if (entry)
    {
      vfatSplitPathName(PathNameU, &DirNameU, &FileNameU);
    }

  while (entry)
    {
      if (entry->Hash == Hash)
        {
          rcFCB = entry->self;
	  DPRINT("'%wZ' '%wZ'\n", &DirNameU, &rcFCB->DirNameU);
	  if (RtlEqualUnicodeString(&DirNameU, &rcFCB->DirNameU, TRUE))
	    {
	      if (rcFCB->Hash.Hash == Hash)
	        {
	          FcbNameU = &rcFCB->LongNameU;
	        }
	      else
	        {
	          FcbNameU = &rcFCB->ShortNameU;
	        }
	      /* compare the file name */
	      DPRINT("'%wZ' '%wZ'\n", &FileNameU, FcbNameU);
	      if (RtlEqualUnicodeString(&FileNameU, FcbNameU, TRUE))
	        {
                  rcFCB->RefCount++;
		  CHECKPOINT;
                  return rcFCB;
	        }
	    }
	}
     entry = entry->next;
  }
  CHECKPOINT;
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
  RtlZeroMemory(newCCB, sizeof (VFATCCB));

  fileObject->SectionObjectPointer = &fcb->SectionObjectPointers;
  fileObject->FsContext = fcb;
  fileObject->FsContext2 = newCCB;
  fcb->FileObject = fileObject;


  fileCacheQuantum = (vcb->FatInfo.BytesPerCluster >= PAGE_SIZE) ?
      vcb->FatInfo.BytesPerCluster : PAGE_SIZE;

  status = CcRosInitializeFileCache (fileObject,
                                     fileCacheQuantum);
  if (!NT_SUCCESS (status))
  {
    DbgPrint ("CcRosInitializeFileCache failed\n");
    KEBUGCHECK (0);
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
  UNICODE_STRING NameU;

  RtlRosInitUnicodeStringFromLiteral(&NameU, L"\\");

  FCB = vfatNewFCB(pVCB, &NameU);
  if (FCB->Flags & FCB_IS_FATX_ENTRY)
  {
    memset(FCB->entry.FatX.Filename, ' ', 42);
    FCB->entry.FatX.FileSize = pVCB->FatInfo.rootDirectorySectors * pVCB->FatInfo.BytesPerSector;
    FCB->entry.FatX.Attrib = FILE_ATTRIBUTE_DIRECTORY;
    FCB->entry.FatX.FirstCluster = 1;
    Size = pVCB->FatInfo.rootDirectorySectors * pVCB->FatInfo.BytesPerSector;
  }
  else
  {
    memset(FCB->entry.Fat.Filename, ' ', 11);
    FCB->entry.Fat.FileSize = pVCB->FatInfo.rootDirectorySectors * pVCB->FatInfo.BytesPerSector;
    FCB->entry.Fat.Attrib = FILE_ATTRIBUTE_DIRECTORY;
    if (pVCB->FatInfo.FatType == FAT32)
    {
      CurrentCluster = FirstCluster = pVCB->FatInfo.RootCluster;
      FCB->entry.Fat.FirstCluster = (unsigned short)(FirstCluster & 0xffff);
      FCB->entry.Fat.FirstClusterHigh = (unsigned short)(FirstCluster >> 16);

      while (CurrentCluster != 0xffffffff && NT_SUCCESS(Status))
      {
        Size += pVCB->FatInfo.BytesPerCluster;
        Status = NextCluster (pVCB, FirstCluster, &CurrentCluster, FALSE);
      }
    }
    else
    {
      FCB->entry.Fat.FirstCluster = 1;
      Size = pVCB->FatInfo.rootDirectorySectors * pVCB->FatInfo.BytesPerSector;
    }
  }
  FCB->ShortHash.Hash = FCB->Hash.Hash;
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
  UNICODE_STRING NameU;

  RtlRosInitUnicodeStringFromLiteral(&NameU, L"\\");

  FCB = vfatGrabFCBFromTable (pVCB, &NameU);
  if (FCB == NULL)
  {
    FCB = vfatMakeRootFCB (pVCB);
  }

  return  FCB;
}

NTSTATUS
vfatMakeFCBFromDirEntry(PVCB  vcb,
			PVFATFCB  directoryFCB,
			PVFAT_DIRENTRY_CONTEXT DirContext,
			PVFATFCB* fileFCB)
{
  PVFATFCB  rcFCB;
  PWCHAR PathNameBuffer;
  ULONG PathNameLength;
  ULONG Size;
  ULONG hash;
  
  UNICODE_STRING NameU;

  PathNameLength = (directoryFCB->PathNameU.Length + 2 * sizeof(WCHAR)
                   + DirContext->LongNameU.Length) * sizeof(WCHAR);
  
  if (PathNameLength > (USHRT_MAX - 2) * sizeof(WCHAR))
  {
    return  STATUS_OBJECT_NAME_INVALID;
  }
  PathNameBuffer = ExAllocatePool(NonPagedPool, PathNameLength);
  if (!PathNameBuffer)
  {
    return STATUS_INSUFFICIENT_RESOURCES;
  }
  NameU.Buffer = PathNameBuffer;
  NameU.Length = 0;
  NameU.MaximumLength = PathNameLength;

  RtlCopyUnicodeString(&NameU, &directoryFCB->PathNameU);
  if (!vfatFCBIsRoot (directoryFCB))
  {
    RtlAppendUnicodeToString(&NameU, L"\\");
  }
  hash = vfatNameHash(0, &NameU);
  if (DirContext->LongNameU.Length > 0)
  {
    RtlAppendUnicodeStringToString(&NameU, &DirContext->LongNameU);
  }
  else
  {
    RtlAppendUnicodeStringToString(&NameU, &DirContext->ShortNameU);
  }
  NameU.Buffer[NameU.Length / sizeof(WCHAR)] = 0;
  
  rcFCB = vfatNewFCB (vcb, &NameU);
  RtlCopyMemory (&rcFCB->entry, &DirContext->DirEntry, sizeof (DIR_ENTRY));
  RtlCopyUnicodeString(&rcFCB->ShortNameU, &DirContext->ShortNameU);
  if (vcb->Flags & VCB_IS_FATX)
  {
    rcFCB->ShortHash.Hash = rcFCB->Hash.Hash;
  }
  else
  {
    rcFCB->ShortHash.Hash = vfatNameHash(hash, &rcFCB->ShortNameU);
  }

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
    else if (FirstCluster != 0)
    {
      CurrentCluster = FirstCluster;
      while (CurrentCluster != 0xffffffff)
      {
         Size += vcb->FatInfo.BytesPerCluster;
         Status = NextCluster (vcb, FirstCluster, &CurrentCluster, FALSE);
      }
    }
  }
  else if (rcFCB->Flags & FCB_IS_FATX_ENTRY)
  {
    Size = rcFCB->entry.FatX.FileSize;
  }
  else
  {
    Size = rcFCB->entry.Fat.FileSize;
  }
  rcFCB->dirIndex = DirContext->DirIndex;
  rcFCB->startIndex = DirContext->StartIndex;
  rcFCB->RFCB.FileSize.QuadPart = Size;
  rcFCB->RFCB.ValidDataLength.QuadPart = Size;
  rcFCB->RFCB.AllocationSize.QuadPart = ROUND_UP(Size, vcb->FatInfo.BytesPerCluster);
  rcFCB->RefCount++;
  if (vfatFCBIsDirectory(rcFCB))
    {
      vfatFCBInitializeCacheFromVolume(vcb, rcFCB);
    }
  rcFCB->parentFcb = directoryFCB;
  vfatAddFCBToTable (vcb, rcFCB);
  *fileFCB = rcFCB;

  ExFreePool(PathNameBuffer);
  return  STATUS_SUCCESS;
}

NTSTATUS
vfatAttachFCBToFileObject (PDEVICE_EXTENSION  vcb,
                           PVFATFCB  fcb,
                           PFILE_OBJECT  fileObject)
{
  PVFATCCB  newCCB;

  newCCB = ExAllocateFromNPagedLookasideList(&VfatGlobalData->CcbLookasideList);
  if (newCCB == NULL)
  {
    CHECKPOINT;
    return  STATUS_INSUFFICIENT_RESOURCES;
  }
  RtlZeroMemory (newCCB, sizeof (VFATCCB));

  fileObject->SectionObjectPointer = &fcb->SectionObjectPointers;
  fileObject->FsContext = fcb;
  fileObject->FsContext2 = newCCB;
  DPRINT ("file open: fcb:%x PathName:%wZ file size: %d\n", fcb, &fcb->PathNameU, fcb->entry.FileSize);

  return  STATUS_SUCCESS;
}

NTSTATUS
vfatDirFindFile (PDEVICE_EXTENSION  pDeviceExt,
                 PVFATFCB  pDirectoryFCB,
                 PUNICODE_STRING FileToFindU,
                 PVFATFCB * pFoundFCB)
{
  NTSTATUS  status;
  PVOID Context = NULL;
  PVOID Page = NULL;
  BOOLEAN First = TRUE;
  VFAT_DIRENTRY_CONTEXT DirContext;
  WCHAR LongNameBuffer[LONGNAME_MAX_LENGTH];
  WCHAR ShortNameBuffer[13];
  BOOLEAN FoundLong = FALSE;
  BOOLEAN FoundShort = FALSE;

  ASSERT(pDeviceExt);
  ASSERT(pDirectoryFCB);
  ASSERT(FileToFindU);

  DPRINT ("vfatDirFindFile(VCB:%08x, dirFCB:%08x, File:%wZ)\n",
          pDeviceExt,
          pDirectoryFCB,
          FileToFindU);
  DPRINT ("Dir Path:%wZ\n", &pDirectoryFCB->PathNameU);

  DirContext.DirIndex = 0;
  DirContext.LongNameU.Buffer = LongNameBuffer;
  DirContext.LongNameU.Length = 0;
  DirContext.LongNameU.MaximumLength = sizeof(LongNameBuffer);
  DirContext.ShortNameU.Buffer = ShortNameBuffer;
  DirContext.ShortNameU.Length = 0;
  DirContext.ShortNameU.MaximumLength = sizeof(ShortNameBuffer);

  while (TRUE)
    {
      status = pDeviceExt->GetNextDirEntry(&Context,
	                           &Page, 
	                           pDirectoryFCB,
				   &DirContext,
				   First);
      First = FALSE;
      if (status == STATUS_NO_MORE_ENTRIES)
        {
          return STATUS_OBJECT_NAME_NOT_FOUND;
        }
      if (!NT_SUCCESS(status))
        {
	  return status;
	}

      DPRINT ("  Index:%d  longName:%wZ\n",
              DirContext.DirIndex,
              &DirContext.LongNameU);
      DirContext.LongNameU.Buffer[DirContext.LongNameU.Length / sizeof(WCHAR)] = 0;
      DirContext.ShortNameU.Buffer[DirContext.ShortNameU.Length / sizeof(WCHAR)] = 0;
      if (!ENTRY_VOLUME(pDeviceExt, &DirContext.DirEntry))
        {
	  FoundLong = RtlEqualUnicodeString(FileToFindU, &DirContext.LongNameU, TRUE);
	  if (FoundLong == FALSE)
	    {
	      FoundShort = RtlEqualUnicodeString(FileToFindU, &DirContext.ShortNameU, TRUE);
	    }
	  if (FoundLong || FoundShort)
	    {
              status = vfatMakeFCBFromDirEntry (pDeviceExt,
                                                pDirectoryFCB,
						&DirContext,
                                                pFoundFCB);
	      CcUnpinData(Context);
              return  status;
            }
	}
      DirContext.DirIndex++;
  }

  return  STATUS_OBJECT_NAME_NOT_FOUND;
}

NTSTATUS
vfatGetFCBForFile (PDEVICE_EXTENSION  pVCB,
                   PVFATFCB  *pParentFCB,
                   PVFATFCB  *pFCB,
                   PUNICODE_STRING pFileNameU)
{
  NTSTATUS  status;
  PVFATFCB  FCB = NULL;
  PVFATFCB  parentFCB;
  UNICODE_STRING NameU;
  UNICODE_STRING RootNameU;
  PWCHAR curr, prev, last;
  ULONG Length;

  DPRINT ("vfatGetFCBForFile (%x,%x,%x,%wZ)\n",
          pVCB,
          pParentFCB,
          pFCB,
          pFileNameU);

  parentFCB = *pParentFCB;

  if (parentFCB == NULL)
  {
     RtlRosInitUnicodeStringFromLiteral(&RootNameU, L"\\");

     //  Trivial case, open of the root directory on volume
     if (RtlEqualUnicodeString(pFileNameU, &RootNameU, FALSE))
     {
        DPRINT ("returning root FCB\n");

        FCB = vfatOpenRootFCB (pVCB);
        *pFCB = FCB;
        *pParentFCB = NULL;

        return  (FCB != NULL) ? STATUS_SUCCESS : STATUS_OBJECT_PATH_NOT_FOUND;
     }

     /* Check for an existing FCB */
     FCB = vfatGrabFCBFromTable (pVCB, pFileNameU);
     if (FCB)
     {
        *pFCB = FCB;
        *pParentFCB = FCB->parentFcb;
        (*pParentFCB)->RefCount++;
        return STATUS_SUCCESS;
     }

     last = curr = pFileNameU->Buffer + pFileNameU->Length / sizeof(WCHAR) - 1;
     while (*curr != L'\\' && curr > pFileNameU->Buffer)
     {
        curr--;
     }
 
     if (curr > pFileNameU->Buffer)
     {
        NameU.Buffer = pFileNameU->Buffer;
        NameU.MaximumLength = NameU.Length = (curr - pFileNameU->Buffer) * sizeof(WCHAR);
        FCB = vfatGrabFCBFromTable(pVCB, &NameU);
        if (FCB)
        {
	   Length = (curr - pFileNameU->Buffer) * sizeof(WCHAR);
           if (Length != FCB->PathNameU.Length)
	   {
	      if (pFileNameU->Length + FCB->PathNameU.Length - Length > pFileNameU->MaximumLength)
	      {
		 vfatReleaseFCB (pVCB, FCB);
		 return STATUS_OBJECT_NAME_INVALID;
              }
	      RtlMoveMemory(pFileNameU->Buffer + FCB->PathNameU.Length / sizeof(WCHAR), 
		      curr, pFileNameU->Length - Length);
	      pFileNameU->Length += (USHORT)(FCB->PathNameU.Length - Length);
	      curr = pFileNameU->Buffer + FCB->PathNameU.Length / sizeof(WCHAR);
              last = pFileNameU->Buffer + pFileNameU->Length / sizeof(WCHAR) - 1;
	  }
	  RtlCopyMemory(pFileNameU->Buffer, FCB->PathNameU.Buffer, FCB->PathNameU.Length);
        }
     }
     else
     {
       FCB = NULL;
     }

     if (FCB == NULL)
     {
       FCB = vfatOpenRootFCB(pVCB);
       curr = pFileNameU->Buffer;
     }
  
     parentFCB = NULL;
     prev = curr;
  }
  else
  {
     FCB = parentFCB;
     parentFCB = NULL;
     prev = curr = pFileNameU->Buffer - 1;
     last = pFileNameU->Buffer + pFileNameU->Length / sizeof(WCHAR) - 1;
  }

  while (curr <= last)
    {
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
          FCB = NULL;
          *pParentFCB = NULL;
          *pFCB = NULL;

          return  STATUS_OBJECT_PATH_NOT_FOUND;
	}
      parentFCB = FCB;
      if (prev < curr)
        {
	  Length = (curr - prev) * sizeof(WCHAR);
	  if (Length != parentFCB->LongNameU.Length)
	    {
	      if (pFileNameU->Length + parentFCB->LongNameU.Length - Length > pFileNameU->MaximumLength)
	        {
		  vfatReleaseFCB (pVCB, parentFCB);
		  return STATUS_OBJECT_NAME_INVALID;
		}
	      RtlMoveMemory(prev + parentFCB->LongNameU.Length / sizeof(WCHAR), curr, 
		      pFileNameU->Length - (curr - pFileNameU->Buffer) * sizeof(WCHAR));
	      pFileNameU->Length += (USHORT)(parentFCB->LongNameU.Length - Length);
	      curr = prev + parentFCB->LongNameU.Length / sizeof(WCHAR);
              last = pFileNameU->Buffer + pFileNameU->Length / sizeof(WCHAR) - 1;
	    }
	  RtlCopyMemory(prev, parentFCB->LongNameU.Buffer, parentFCB->LongNameU.Length);
	}
      curr++;      
      prev = curr;
      while (*curr != L'\\' && curr <= last)
        {
	  curr++;
	}
      NameU.Buffer = pFileNameU->Buffer;
      NameU.Length = (curr - NameU.Buffer) * sizeof(WCHAR);
      NameU.MaximumLength = pFileNameU->MaximumLength;
      DPRINT("%wZ\n", &NameU);
      FCB = vfatGrabFCBFromTable(pVCB, &NameU);
      if (FCB == NULL)
        {
	  NameU.Buffer = prev;
          NameU.MaximumLength = NameU.Length = (curr - prev) * sizeof(WCHAR);
	  status = vfatDirFindFile(pVCB, parentFCB, &NameU, &FCB);
          if (status == STATUS_OBJECT_NAME_NOT_FOUND)
            {
              *pFCB = NULL;
	      if (curr > last)
	        {
                  *pParentFCB = parentFCB;
                  return  STATUS_OBJECT_NAME_NOT_FOUND;
		}
              else
                {
                  vfatReleaseFCB (pVCB, parentFCB);
		  *pParentFCB = NULL;
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
    }

  *pParentFCB = parentFCB;
  *pFCB = FCB;

  return  STATUS_SUCCESS;
}

