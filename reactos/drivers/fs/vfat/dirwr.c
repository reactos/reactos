/* $Id: dirwr.c,v 1.42 2004/11/06 13:44:57 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/dirwr.c
 * PURPOSE:          VFAT Filesystem : write in directory
 *
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ctype.h>
#include <wchar.h>
#include <string.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

NTSTATUS 
VfatUpdateEntry (PVFATFCB pFcb)
/*
 * update an existing FAT entry
 */
{
  PVOID Context;
  PFAT_DIR_ENTRY PinEntry;
  LARGE_INTEGER Offset;

  ASSERT(pFcb);
  ASSERT(pFcb->parentFcb);

  DPRINT ("updEntry PathName \'%wZ\'\n", &pFcb->PathNameU);

  Offset.u.HighPart = 0;
  Offset.u.LowPart = pFcb->dirIndex * sizeof(FATDirEntry);
  if (CcMapData (pFcb->parentFcb->FileObject, &Offset, sizeof(FATDirEntry),
      TRUE, &Context, (PVOID*)&PinEntry))
    {
      pFcb->Flags &= ~FCB_IS_DIRTY;
      *PinEntry = pFcb->entry;
      CcSetDirtyPinnedData(Context, NULL);
      CcUnpinData(Context);
      return STATUS_SUCCESS;
    }
  else
    {
      DPRINT1 ("Failed write to \'%wZ\'.\n", &pFcb->parentFcb->PathNameU);
      return STATUS_UNSUCCESSFUL;
    }
}

BOOLEAN
vfatFindDirSpace(PDEVICE_EXTENSION DeviceExt,
                 PVFATFCB pDirFcb,
                 ULONG nbSlots,
                 PULONG start)
{
/*
 * try to find contiguous entries frees in directory,
 * extend a directory if is neccesary
 */
  LARGE_INTEGER FileOffset;
  ULONG i, count, size, nbFree = 0;
  FATDirEntry* pFatEntry;
  PVOID Context = NULL;
  NTSTATUS Status;
  FileOffset.QuadPart = 0;
  count = pDirFcb->RFCB.FileSize.u.LowPart / sizeof(FATDirEntry);
  size = DeviceExt->FatInfo.BytesPerCluster / sizeof(FATDirEntry);
  for (i = 0; i < count; i++, pFatEntry++)
  {
    if (Context == NULL || (i % size) == 0)
    {
      if (Context)
      {
        CcUnpinData(Context);
      }
      // FIXME: check return value
      CcMapData (pDirFcb->FileObject, &FileOffset, DeviceExt->FatInfo.BytesPerCluster,
                 TRUE, &Context, (PVOID*)&pFatEntry);
      FileOffset.u.LowPart += DeviceExt->FatInfo.BytesPerCluster;
    }
    if (ENTRY_END(pFatEntry))
    {
      break;
    }
    if (ENTRY_DELETED(pFatEntry))
    {
      nbFree++;
    }
    else
    {
      nbFree = 0;
    }
    if (nbFree == nbSlots)
    {
      break;
    }
  }
  if (Context)
  {
    CcUnpinData(Context);
    Context = NULL;
  }
  if (nbFree == nbSlots)
  {
    // found enough contiguous free slots
    *start = i - nbSlots + 1;
  }
  else
  {
    *start = i - nbFree;
    if (*start + nbSlots > count)
    {
      LARGE_INTEGER AllocationSize;
      CHECKPOINT;
      // extend the directory
      if (vfatFCBIsRoot(pDirFcb) && DeviceExt->FatInfo.FatType != FAT32)
      {
        // We can't extend a root directory on a FAT12/FAT16 partition
        return FALSE;
      }
      AllocationSize.QuadPart = pDirFcb->RFCB.FileSize.u.LowPart + DeviceExt->FatInfo.BytesPerCluster;
      Status = VfatSetAllocationSizeInformation(pDirFcb->FileObject, pDirFcb,
	                                        DeviceExt, &AllocationSize);
      if (!NT_SUCCESS(Status))
      {
        return FALSE;
      }
      // clear the new dir cluster
      FileOffset.u.LowPart = (DWORD)(pDirFcb->RFCB.FileSize.QuadPart -
                                     DeviceExt->FatInfo.BytesPerCluster);
      CcMapData (pDirFcb->FileObject, &FileOffset, DeviceExt->FatInfo.BytesPerCluster,
                 TRUE, &Context, (PVOID*)&pFatEntry);
      RtlZeroMemory(pFatEntry, DeviceExt->FatInfo.BytesPerCluster);
    }
    else if (*start + nbSlots < count)
    {
      // clear the entry after the last new entry
      FileOffset.u.LowPart = (*start + nbSlots) * sizeof(FATDirEntry);
      CcMapData (pDirFcb->FileObject, &FileOffset, sizeof(FATDirEntry),
                 TRUE, &Context, (PVOID*)&pFatEntry);
      RtlZeroMemory(pFatEntry, sizeof(FATDirEntry));
    }
    if (Context)
    {
      CcSetDirtyPinnedData(Context, NULL);
      CcUnpinData(Context);
    }
  }
  DPRINT ("nbSlots %d nbFree %d, entry number %d\n", nbSlots, nbFree, *start);
  return TRUE;
}

NTSTATUS
VfatAddEntry (PDEVICE_EXTENSION DeviceExt,
	      PUNICODE_STRING PathNameU,
	      PFILE_OBJECT pFileObject,
	      ULONG RequestedOptions,
	      UCHAR ReqAttr)
/*
  create a new FAT entry
*/
{
  PVOID Context = NULL;
  FATDirEntry *pFatEntry;
  slot *pSlots;
  short nbSlots = 0, j, posCar;
  PUCHAR Buffer;
  BOOLEAN needTilde = FALSE, needLong = FALSE;
  BOOLEAN lCaseBase = FALSE, uCaseBase, lCaseExt = FALSE, uCaseExt;
  PVFATFCB newFCB;
  ULONG CurrentCluster;
  LARGE_INTEGER SystemTime, FileOffset;
  NTSTATUS Status = STATUS_SUCCESS;
  PVFATFCB pDirFcb;
  ULONG size;
  long i;
  
  ANSI_STRING NameA;
  CHAR aName[13];
  BOOLEAN IsNameLegal;
  BOOLEAN SpacesFound;

  VFAT_DIRENTRY_CONTEXT DirContext;
  UNICODE_STRING DirNameU;
  WCHAR LongNameBuffer[MAX_PATH];
  WCHAR ShortNameBuffer[13];

  DPRINT ("addEntry: Pathname='%wZ'\n", PathNameU);

  vfatSplitPathName(PathNameU, &DirNameU, &DirContext.LongNameU);
  if (DirNameU.Length > sizeof(WCHAR))
    {
      DirNameU.Length -= sizeof(WCHAR);
    }

  pDirFcb = vfatGrabFCBFromTable(DeviceExt, &DirNameU);
  if (pDirFcb == NULL)
    {
      return STATUS_UNSUCCESSFUL;
    }

  if (!ExAcquireResourceExclusiveLite(&pDirFcb->MainResource, TRUE))
    {
      DPRINT("Failed acquiring lock\n");
      return STATUS_UNSUCCESSFUL;
    }
  
  nbSlots = (DirContext.LongNameU.Length / sizeof(WCHAR) + 12) / 13 + 1;	//nb of entry needed for long name+normal entry
  DPRINT ("NameLen= %d, nbSlots =%d\n", DirContext.LongNameU.Length / sizeof(WCHAR), nbSlots);
  Buffer = ExAllocatePool (NonPagedPool, (nbSlots - 1) * sizeof (FATDirEntry));
  RtlZeroMemory (Buffer, (nbSlots - 1) * sizeof (FATDirEntry));
  pSlots = (slot *) Buffer;

  NameA.Buffer = aName;
  NameA.Length = 0;
  NameA.MaximumLength = sizeof(aName);

  DirContext.ShortNameU.Buffer = ShortNameBuffer;
  DirContext.ShortNameU.Length = 0;
  DirContext.ShortNameU.MaximumLength = sizeof(ShortNameBuffer);

  memset(&DirContext.FatDirEntry, 0, sizeof(FATDirEntry));

  IsNameLegal = RtlIsNameLegalDOS8Dot3(&DirContext.LongNameU, &NameA, &SpacesFound);

  if (IsNameLegal == FALSE || SpacesFound != FALSE)
    {
      GENERATE_NAME_CONTEXT NameContext;
      VFAT_DIRENTRY_CONTEXT SearchContext;
      WCHAR ShortSearchName[13];
      needTilde = TRUE;
      needLong = TRUE;
      memset(&NameContext, 0, sizeof(GENERATE_NAME_CONTEXT));
      SearchContext.LongNameU.Buffer = LongNameBuffer;
      SearchContext.LongNameU.MaximumLength = sizeof(LongNameBuffer);
      SearchContext.ShortNameU.Buffer = ShortSearchName;
      SearchContext.ShortNameU.MaximumLength = sizeof(ShortSearchName);

      for (i = 0; i < 100; i++)
        {
          RtlGenerate8dot3Name(&DirContext.LongNameU, FALSE, &NameContext, &DirContext.ShortNameU);
          DirContext.ShortNameU.Buffer[DirContext.ShortNameU.Length / sizeof(WCHAR)] = 0;
	  SearchContext.DirIndex = 0;
          Status = FindFile (DeviceExt, pDirFcb, &DirContext.ShortNameU, &SearchContext, TRUE);
          if (!NT_SUCCESS(Status))
            {
	      break;
            }
        }
      if (i == 100) /* FIXME : what to do after this ? */
        {
          ExReleaseResourceLite(&pDirFcb->MainResource);
          vfatReleaseFCB(DeviceExt, pDirFcb);
          ExFreePool (Buffer);
	  CHECKPOINT;
          return STATUS_UNSUCCESSFUL;
        }
      IsNameLegal = RtlIsNameLegalDOS8Dot3(&DirContext.ShortNameU, &NameA, &SpacesFound);
      aName[NameA.Length]=0;
    }
  else
    {
      aName[NameA.Length] = 0;
      for (posCar = 0; posCar < DirContext.LongNameU.Length / sizeof(WCHAR); posCar++)
        {
          if (DirContext.LongNameU.Buffer[posCar] == L'.')
	    {
	      break;
	    }
        }
      /* check if the name and the extension contains upper case characters */
      RtlDowncaseUnicodeString(&DirContext.ShortNameU, &DirContext.LongNameU, FALSE);
      DirContext.ShortNameU.Buffer[DirContext.ShortNameU.Length / sizeof(WCHAR)] = 0;
      uCaseBase = wcsncmp(DirContext.LongNameU.Buffer, 
	                  DirContext.ShortNameU.Buffer, posCar) ? TRUE : FALSE;
      if (posCar < DirContext.LongNameU.Length/sizeof(WCHAR))
        {
	  i = DirContext.LongNameU.Length / sizeof(WCHAR) - posCar;
	  uCaseExt = wcsncmp(DirContext.LongNameU.Buffer + posCar, 
	                     DirContext.ShortNameU.Buffer + posCar, i) ? TRUE : FALSE;
        }
      else
        {
          uCaseExt = FALSE;
        }
      /* check if the name and the extension contains lower case characters */
      RtlUpcaseUnicodeString(&DirContext.ShortNameU, &DirContext.LongNameU, FALSE);
      DirContext.ShortNameU.Buffer[DirContext.ShortNameU.Length / sizeof(WCHAR)] = 0;
      lCaseBase = wcsncmp(DirContext.LongNameU.Buffer, 
	                  DirContext.ShortNameU.Buffer, posCar) ? TRUE : FALSE;
      if (posCar < DirContext.LongNameU.Length / sizeof(WCHAR))
        {
	  i = DirContext.LongNameU.Length / sizeof(WCHAR) - posCar;
	  lCaseExt = wcsncmp(DirContext.LongNameU.Buffer + posCar, 
	                     DirContext.ShortNameU.Buffer + posCar, i) ? TRUE : FALSE;
        }
      else
        {
	  lCaseExt = FALSE;
        }
      if ((lCaseBase && uCaseBase) || (lCaseExt && uCaseExt))
        {
          needLong = TRUE;
        }
    }
  DPRINT ("'%s', '%wZ', needTilde=%d, needLong=%d\n", 
          aName, &DirContext.LongNameU, needTilde, needLong);
  memset(DirContext.FatDirEntry.Filename, ' ', 11);
  for (i = 0; i < 8 && aName[i] && aName[i] != '.'; i++)
    {
      DirContext.FatDirEntry.Filename[i] = aName[i];
    }
  if (aName[i] == '.')
    {
      i++;
      for (j = 8; j < 11 && aName[i]; j++, i++)
        {
          DirContext.FatDirEntry.Filename[j] = aName[i];
        }
    }
  if (DirContext.FatDirEntry.Filename[0] == 0xe5)
    {
      DirContext.FatDirEntry.Filename[0] = 0x05;
    }

  if (needLong)
    {
      memcpy(LongNameBuffer, DirContext.LongNameU.Buffer, DirContext.LongNameU.Length);
      DirContext.LongNameU.Buffer = LongNameBuffer;
      DirContext.LongNameU.MaximumLength = sizeof(LongNameBuffer);
      DirContext.LongNameU.Buffer[DirContext.LongNameU.Length / sizeof(WCHAR)] = 0;
      memset(DirContext.LongNameU.Buffer + DirContext.LongNameU.Length / sizeof(WCHAR) + 1, 0xff, 
	     DirContext.LongNameU.MaximumLength - DirContext.LongNameU.Length - sizeof(WCHAR));
    }
  else
    {
      nbSlots = 1;
      if (lCaseBase)
        {
	  DirContext.FatDirEntry.lCase |= VFAT_CASE_LOWER_BASE;
        }
      if (lCaseExt)
        {
	  DirContext.FatDirEntry.lCase |= VFAT_CASE_LOWER_EXT;
        }
    }

  DPRINT ("dos name=%11.11s\n", DirContext.FatDirEntry.Filename);

  /* set attributes */
  DirContext.FatDirEntry.Attrib = ReqAttr;
  if (RequestedOptions & FILE_DIRECTORY_FILE)
    {
      DirContext.FatDirEntry.Attrib |= FILE_ATTRIBUTE_DIRECTORY;
    }
  /* set dates and times */
  KeQuerySystemTime (&SystemTime);
#if 0
  {
    TIME_FIELDS tf;
    RtlTimeToTimeFields (&SystemTime, &tf);
    DPRINT1("%d.%d.%d %02d:%02d:%02d.%03d '%S'\n", 
	    tf.Day, tf.Month, tf.Year, tf.Hour, 
	    tf.Minute, tf.Second, tf.Milliseconds,
	    pFileObject->FileName.Buffer);
  }
#endif
  FsdSystemTimeToDosDateTime (&SystemTime, &DirContext.FatDirEntry.CreationDate,
                              &DirContext.FatDirEntry.CreationTime);
  DirContext.FatDirEntry.UpdateDate = DirContext.FatDirEntry.CreationDate;
  DirContext.FatDirEntry.UpdateTime = DirContext.FatDirEntry.CreationTime;
  DirContext.FatDirEntry.AccessDate = DirContext.FatDirEntry.CreationDate;

  if (needLong)
    {
      /* calculate checksum for 8.3 name */
      for (pSlots[0].alias_checksum = 0, i = 0; i < 11; i++)
        {
          pSlots[0].alias_checksum = (((pSlots[0].alias_checksum & 1) << 7
                                   | ((pSlots[0].alias_checksum & 0xfe) >> 1))
                                   + DirContext.FatDirEntry.Filename[i]);
        }
      /* construct slots and entry */
      for (i = nbSlots - 2; i >= 0; i--)
        {
          DPRINT ("construct slot %d\n", i);
          pSlots[i].attr = 0xf;
          if (i)
            {
              pSlots[i].id = nbSlots - i - 1;
            }
          else
            {
              pSlots[i].id = nbSlots - i - 1 + 0x40;
            }
          pSlots[i].alias_checksum = pSlots[0].alias_checksum;
          memcpy (pSlots[i].name0_4, DirContext.LongNameU.Buffer + (nbSlots - i - 2) * 13, 10);
          memcpy (pSlots[i].name5_10, DirContext.LongNameU.Buffer + (nbSlots - i - 2) * 13 + 5, 12);
          memcpy (pSlots[i].name11_12, DirContext.LongNameU.Buffer + (nbSlots - i - 2) * 13 + 11, 4);
	}
    }
  /* try to find nbSlots contiguous entries frees in directory */
  if (!vfatFindDirSpace(DeviceExt, pDirFcb, nbSlots, &DirContext.StartIndex))
    {
      ExReleaseResourceLite(&pDirFcb->MainResource);
      vfatReleaseFCB(DeviceExt, pDirFcb);
      ExFreePool (Buffer);
      return STATUS_DISK_FULL;
    }
  DirContext.DirIndex = DirContext.StartIndex + nbSlots - 1;
  if (RequestedOptions & FILE_DIRECTORY_FILE)
    {
      CurrentCluster = 0;
      Status = NextCluster (DeviceExt, 0, &CurrentCluster, TRUE);
      if (CurrentCluster == 0xffffffff || !NT_SUCCESS(Status))
        {
          ExReleaseResourceLite(&pDirFcb->MainResource);
          vfatReleaseFCB(DeviceExt, pDirFcb);
          ExFreePool (Buffer);
          if (!NT_SUCCESS(Status))
            {
              return Status;
            }
          return STATUS_DISK_FULL;
        }
      if (DeviceExt->FatInfo.FatType == FAT32)
        {
          DirContext.FatDirEntry.FirstClusterHigh = (unsigned short)(CurrentCluster >> 16);
        }
      DirContext.FatDirEntry.FirstCluster = (unsigned short)CurrentCluster;
    }

  i = DeviceExt->FatInfo.BytesPerCluster / sizeof(FATDirEntry);
  FileOffset.u.HighPart = 0;
  FileOffset.u.LowPart = DirContext.StartIndex * sizeof(FATDirEntry);
  if (DirContext.StartIndex / i == DirContext.DirIndex / i)
    {
      /* one cluster */
      CHECKPOINT;
      CcMapData (pDirFcb->FileObject, &FileOffset, nbSlots * sizeof(FATDirEntry),
                 TRUE, &Context, (PVOID*)&pFatEntry);
      if (nbSlots > 1)
        {
          memcpy(pFatEntry, Buffer, (nbSlots - 1) * sizeof(FATDirEntry));
        }
      memcpy(pFatEntry + (nbSlots - 1), &DirContext.FatDirEntry, sizeof(FATDirEntry));
    }
  else
    {
      /* two clusters */
      CHECKPOINT;
      size = DeviceExt->FatInfo.BytesPerCluster -
             (DirContext.StartIndex * sizeof(FATDirEntry)) % DeviceExt->FatInfo.BytesPerCluster;
      i = size / sizeof(FATDirEntry);
      CcMapData (pDirFcb->FileObject, &FileOffset, size, TRUE,
                 &Context, (PVOID*)&pFatEntry);
      memcpy(pFatEntry, Buffer, size);
      CcSetDirtyPinnedData(Context, NULL);
      CcUnpinData(Context);
      FileOffset.u.LowPart += size;
      CcMapData (pDirFcb->FileObject, &FileOffset,
                 nbSlots * sizeof(FATDirEntry) - size,
                 TRUE, &Context, (PVOID*)&pFatEntry);
      if (nbSlots - 1 > i)
        {
          memcpy(pFatEntry, (PVOID)(Buffer + size), (nbSlots - 1 - i) * sizeof(FATDirEntry));
        }
      memcpy(pFatEntry + nbSlots - 1 - i, &DirContext.FatDirEntry, sizeof(FATDirEntry));
    }
  CcSetDirtyPinnedData(Context, NULL);
  CcUnpinData(Context);

  /* FIXME: check status */
  vfatMakeFCBFromDirEntry (DeviceExt, pDirFcb, &DirContext, &newFCB);
  vfatAttachFCBToFileObject (DeviceExt, newFCB, pFileObject);

  DPRINT ("new : entry=%11.11s\n", newFCB->entry.Filename);
  DPRINT ("new : entry=%11.11s\n", DirContext.FatDirEntry.Filename);

  if (RequestedOptions & FILE_DIRECTORY_FILE)
    {
      FileOffset.QuadPart = 0;
      CcMapData (pFileObject, &FileOffset, DeviceExt->FatInfo.BytesPerCluster, TRUE,
                 &Context, (PVOID*)&pFatEntry);
      /* clear the new directory cluster */
      RtlZeroMemory (pFatEntry, DeviceExt->FatInfo.BytesPerCluster);
      /* create '.' and '..' */
      memcpy (&pFatEntry[0].Attrib, &DirContext.FatDirEntry.Attrib, sizeof(FATDirEntry) - 11);
      memcpy (pFatEntry[0].Filename, ".          ", 11);
      memcpy (&pFatEntry[1].Attrib, &DirContext.FatDirEntry.Attrib, sizeof(FATDirEntry) - 11);
      memcpy (pFatEntry[1].Filename, "..         ", 11);
      pFatEntry[1].FirstCluster = pDirFcb->entry.FirstCluster;
      pFatEntry[1].FirstClusterHigh = pDirFcb->entry.FirstClusterHigh;
      if (vfatFCBIsRoot(pDirFcb))
        {
          pFatEntry[1].FirstCluster = 0;
          pFatEntry[1].FirstClusterHigh = 0;
        }
      CcSetDirtyPinnedData(Context, NULL);
      CcUnpinData(Context);
    }
  ExReleaseResourceLite(&pDirFcb->MainResource);
  vfatReleaseFCB (DeviceExt, pDirFcb);
  ExFreePool (Buffer);
  DPRINT ("addentry ok\n");
  return STATUS_SUCCESS;
}

NTSTATUS
VfatDelEntry (PDEVICE_EXTENSION DeviceExt, PVFATFCB pFcb)
/*
 * deleting an existing FAT entry
 */
{
  ULONG CurrentCluster = 0, NextCluster, i;
  PVOID Context = NULL;
  LARGE_INTEGER Offset;
  FATDirEntry* pDirEntry;

  ASSERT(pFcb);
  ASSERT(pFcb->parentFcb);

  DPRINT ("delEntry PathName \'%wZ\'\n", &pFcb->PathNameU);
  DPRINT ("delete entry: %d to %d\n", pFcb->startIndex, pFcb->dirIndex);
  Offset.u.HighPart = 0;
  for (i = pFcb->startIndex; i <= pFcb->dirIndex; i++)
    {
      if (Context == NULL || ((i * sizeof(FATDirEntry)) % PAGE_SIZE) == 0)
        {
          if (Context)
          {
            CcSetDirtyPinnedData(Context, NULL);
            CcUnpinData(Context);
          }
          Offset.u.LowPart = (i * sizeof(FATDirEntry) / PAGE_SIZE) * PAGE_SIZE;
          CcMapData (pFcb->parentFcb->FileObject, &Offset, PAGE_SIZE, TRUE,
                     &Context, (PVOID*)&pDirEntry);
        }
      pDirEntry[i % (PAGE_SIZE / sizeof(FATDirEntry))].Filename[0] = 0xe5;
      if (i == pFcb->dirIndex)
        {
          CurrentCluster =
          vfatDirEntryGetFirstCluster (DeviceExt,
            &pDirEntry[i % (PAGE_SIZE / sizeof(FATDirEntry))]);
        }
    }
  if (Context)
    {
      CcSetDirtyPinnedData(Context, NULL);
      CcUnpinData(Context);
    }

  while (CurrentCluster && CurrentCluster != 0xffffffff)
    {
      GetNextCluster (DeviceExt, CurrentCluster, &NextCluster);
      /* FIXME: check status */
      WriteCluster(DeviceExt, CurrentCluster, 0);
      CurrentCluster = NextCluster;
    }
  return STATUS_SUCCESS;
}

/* EOF */
