/* $Id: dirwr.c,v 1.36 2003/05/11 09:51:26 hbirr Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/dirwr.c
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

const char *short_illegals=" ;+=[]',\"*\\<>/?:|";

static BOOLEAN
vfatIsShortIllegal(char c)
{
  int i;
  for (i = 0; short_illegals[i]; i++)
    if (c == short_illegals[i])
      return TRUE;
  return FALSE;
}

NTSTATUS 
VfatUpdateEntry (PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT pFileObject)
/*
 * update an existing FAT entry
 */
{
  PVOID Context;
  PVOID Buffer;
  PVFATFCB pDirFcb, pFcb;
  LARGE_INTEGER Offset;

  DPRINT ("updEntry PathFileName \'%S\'\n", 
          ((PVFATCCB)(pFileObject->FsContext2))->pFcb->PathName);

  pFcb = (PVFATFCB)pFileObject->FsContext;
  assert (pFcb);
  pDirFcb = pFcb->parentFcb;
  assert (pDirFcb);

  Offset.u.HighPart = 0;
  Offset.u.LowPart = pFcb->dirIndex * sizeof(FATDirEntry);
  if (CcMapData (pDirFcb->FileObject, &Offset, sizeof(FATDirEntry),
    TRUE, &Context, (PVOID*)&Buffer))
  {
     memcpy(Buffer, &pFcb->entry, sizeof(FATDirEntry));
     CcSetDirtyPinnedData(Context, NULL);
     CcUnpinData(Context);
  }
  else
     DPRINT1 ("Failed write to \'%S\'.\n", pDirFcb->PathName);

  return STATUS_SUCCESS;
}

BOOLEAN
findDirSpace(PDEVICE_EXTENSION DeviceExt,
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
    if (vfatIsDirEntryEndMarker(pFatEntry))
    {
      break;
    }
    if (vfatIsDirEntryDeleted(pFatEntry))
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
      FileOffset.u.LowPart = pDirFcb->RFCB.FileSize.QuadPart -
                               DeviceExt->FatInfo.BytesPerCluster;
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
	      PFILE_OBJECT pFileObject,
	      ULONG RequestedOptions,
	      UCHAR ReqAttr)
/*
  create a new FAT entry
*/
{
  WCHAR DirName[MAX_PATH], *FileName, *PathFileName;
  VFATFCB FileFcb;
  PVOID Context = NULL;
  FATDirEntry *pFatEntry, *pEntry;
  slot *pSlots;
  short nbSlots = 0, nbFree = 0, j, posCar, NameLen;
  PUCHAR Buffer;
  BOOLEAN needTilde = FALSE, needLong = FALSE;
  BOOLEAN lCaseBase, uCaseBase, lCaseExt, uCaseExt;
  PVFATFCB newFCB;
  ULONG CurrentCluster;
  LARGE_INTEGER SystemTime, LocalTime, FileOffset;
  NTSTATUS Status = STATUS_SUCCESS;
  PVFATFCB pDirFcb;
  ULONG start, size;
  long i;

  PathFileName = pFileObject->FileName.Buffer;
  DPRINT ("addEntry: Pathname=%S\n", PathFileName);
  //find last \ in PathFileName
  posCar = -1;
  for (i = 0; PathFileName[i]; i++)
  {
    if (PathFileName[i] == L'\\')
    {
      posCar = i;
    }
  }
  if (posCar == -1)
  {
    return STATUS_UNSUCCESSFUL;
  }
  FileName = &PathFileName[posCar + 1];
  for (NameLen = 0; FileName[NameLen]; NameLen++);
  // extract directory name from pathname
  if (posCar == 0)
  {
    // root dir
    DirName[0] = L'\\';
    DirName[1] = 0;
  }
  else
  {
    memcpy (DirName, PathFileName, posCar * sizeof (WCHAR));
    DirName[posCar] = 0;
  }
  // open parent directory
  pDirFcb = vfatGrabFCBFromTable(DeviceExt, DirName);
  if (pDirFcb == NULL)
  {
    return STATUS_UNSUCCESSFUL;
  }
  nbSlots = (NameLen + 12) / 13 + 1;	//nb of entry needed for long name+normal entry
  DPRINT ("NameLen= %d, nbSlots =%d\n", NameLen, nbSlots);
  Buffer = ExAllocatePool (NonPagedPool, nbSlots * sizeof (FATDirEntry));
  RtlZeroMemory (Buffer, nbSlots * sizeof (FATDirEntry));
  pEntry = (FATDirEntry *) (Buffer + (nbSlots - 1) * sizeof (FATDirEntry));
  pSlots = (slot *) Buffer;
  // create 8.3 name
  needTilde = FALSE;
  // find last point in name
  posCar = j = 0;
  for (i = 0; FileName[i]; i++)
  {
    if (FileName[i] == '.')
    {
      posCar = i;
      if (i == j)
      {
        j++;
      }
    }
  }
  if (!posCar)
  {
    posCar = i;
  }
  if (posCar < j)
  {
    posCar = i;
    needTilde = TRUE;
  }
  if (posCar > 8)
  {
    needTilde = TRUE;
  }
  //copy 8 characters max
  memset (pEntry, ' ', 11);
  for (i = 0, j = 0; j < 8 && i < posCar; i++)
  {
    if (vfatIsShortIllegal (FileName[i]))
    {
      needTilde = TRUE;
      pEntry->Filename[j++] = '_';
    }
    else
    {
      if (FileName[i] == '.')
      {
        needTilde = TRUE;
      }
      else
      {
        pEntry->Filename[j++] = toupper ((char) FileName[i]);
      }
    }
  }
  //copy extension
  if (FileName[posCar])
  {
    for (j = 0, i = posCar + 1; FileName[i] && j < 3; i++)
    {
      if (vfatIsShortIllegal(FileName[i]))
      {
        needTilde = TRUE;
        pEntry->Ext[j++] = '_';
      }
      else
      {
        if (FileName[i] == '.')
        {
          needTilde = TRUE;
        }
        else
        {
          pEntry->Ext[j++] = toupper ((char) (FileName[i] & 0x7F));
        }
      }
    }
  }
  if (FileName[i])
  {
    needTilde = TRUE;
  }
  //find good value for tilde
  if (needTilde)
  {
    needLong = TRUE;
    DPRINT ("searching a good value for tilde\n");
    for (posCar = 0; posCar < 8 && pEntry->Filename[posCar] != ' '; posCar++);
    if (posCar == 0) // ??????????????????????
    {
      pEntry->Filename[posCar++] = '_';
    }
    posCar += 2;
    if (posCar > 8)
    {
      posCar = 8;
    }
    pEntry->Filename[posCar - 2] = '~';
    pEntry->Filename[posCar - 1] = '1';
    vfat8Dot3ToString (pEntry, DirName);
    //try first with xxxxxx~y.zzz
    for (i = 1; i < 10; i++)
    {
      DirName[posCar-1] = '0' + i;
      pEntry->Filename[posCar - 1] = '0' + i;
      Status = FindFile (DeviceExt, &FileFcb, pDirFcb, DirName, NULL, NULL);
      if (!NT_SUCCESS(Status))
      {
        break;
      }
    }
    if (i == 10)
    {
      posCar++;
      if (posCar > 8)
      {
        posCar = 8;
      }
      pEntry->Filename[posCar - 3] = '~';
      pEntry->Filename[posCar - 2] = '1';
      pEntry->Filename[posCar - 1] = '0';
      vfat8Dot3ToString (pEntry, DirName);
      //try second with xxxxx~yy.zzz
      for (i = 10; i < 100; i++)
      {
        DirName[posCar - 1] = '0' + i % 10;
        DirName[posCar - 2] = '0' + i / 10;
        pEntry->Filename[posCar - 1] = '0' + i % 10;
        pEntry->Filename[posCar - 2] = '0' + i / 10;
        Status = FindFile (DeviceExt, &FileFcb, pDirFcb, DirName, NULL, NULL);
        if (!NT_SUCCESS(Status))
        {
          break;
        }
      }
      if (i == 100) //FIXME : what to do after 99 tilde ?
      {
        vfatReleaseFCB(DeviceExt, pDirFcb);
        ExFreePool (Buffer);
        return STATUS_UNSUCCESSFUL;
      }
    }
  }
  else
  {
    DPRINT ("check if long name entry needed, needlong=%d\n", needLong);
    lCaseBase = uCaseBase = lCaseExt = uCaseExt = FALSE;
    for (i = 0; i < posCar; i++)
    {
      if ((USHORT) tolower(pEntry->Filename[i]) == FileName[i])
      {
         DPRINT ("i=%d,%d,%d\n", i, pEntry->Filename[i], FileName[i]);
	 lCaseBase = TRUE;
      }
      else if ((USHORT) pEntry->Filename[i] == FileName[i])
      {
         DPRINT ("i=%d,%d,%d\n", i, pEntry->Filename[i], FileName[i]);
	 uCaseBase = TRUE;
      }
      else
      {
        DPRINT ("i=%d,%d,%d\n", i, pEntry->Filename[i], FileName[i]);
        needLong = TRUE;
      }
    }
    if (FileName[i])
    {
      i++;			//jump on point char
      for (j = 0, i = posCar + 1; FileName[i] && i < posCar + 4; i++, j++)
      {
	if ((USHORT) tolower(pEntry->Ext[j]) == FileName[i])
	{
           DPRINT ("i=%d,j=%d,%d,%d\n", i, j, pEntry->Ext[j], FileName[i]);
	   lCaseExt = TRUE;
	}
	else if ((USHORT) pEntry->Ext[j] == FileName[i])
	{
           DPRINT ("i=%d,j=%d,%d,%d\n", i, j, pEntry->Ext[j], FileName[i]);
	   uCaseExt = TRUE;
	}
        else
        {
          DPRINT ("i=%d,j=%d,%d,%d\n", i, j, pEntry->Ext[j], FileName[i]);
          needLong = TRUE;
        }
      }
    }
    if ((lCaseBase && uCaseBase) || (lCaseExt && uCaseExt))
    {
       CHECKPOINT;
       needLong = TRUE;
    }
  }
  if (needLong == FALSE)
  {
    nbSlots = 1;
    memcpy (Buffer, pEntry, sizeof (FATDirEntry));
    memset (pEntry, 0, sizeof (FATDirEntry));
    pEntry = (FATDirEntry *) Buffer;
    if (lCaseBase)
    {
	pEntry->lCase |= VFAT_CASE_LOWER_BASE;
    }
    if (lCaseExt)
    {
	pEntry->lCase |= VFAT_CASE_LOWER_EXT;
    }
  }
  else
  {
    memset (DirName, 0xff, sizeof (DirName));
    memcpy (DirName, FileName, NameLen * sizeof (WCHAR));
    DirName[NameLen] = 0;
  }
  DPRINT ("dos name=%11.11s\n", pEntry->Filename);

  /* set attributes */
  pEntry->Attrib = ReqAttr;
  if (RequestedOptions & FILE_DIRECTORY_FILE)
  {
    pEntry->Attrib |= FILE_ATTRIBUTE_DIRECTORY;
  }
  /* set dates and times */
  KeQuerySystemTime (&SystemTime);
  ExSystemTimeToLocalTime (&SystemTime, &LocalTime);
#if 0
  {
    TIME_FIELDS tf;
    RtlTimeToTimeFields (&LocalTime, &tf);
    DPRINT1("%d.%d.%d %02d:%02d:%02d.%03d '%S'\n", 
	    tf.Day, tf.Month, tf.Year, tf.Hour, 
	    tf.Minute, tf.Second, tf.Milliseconds,
	    pFileObject->FileName.Buffer);
  }
#endif
  FsdFileTimeToDosDateTime ((TIME *) & LocalTime, &pEntry->CreationDate,
                            &pEntry->CreationTime);
  pEntry->UpdateDate = pEntry->CreationDate;
  pEntry->UpdateTime = pEntry->CreationTime;
  pEntry->AccessDate = pEntry->CreationDate;

  if (needLong)
  {
    // calculate checksum for 8.3 name
    for (pSlots[0].alias_checksum = i = 0; i < 11; i++)
    {
       pSlots[0].alias_checksum = (((pSlots[0].alias_checksum & 1) << 7
                                  | ((pSlots[0].alias_checksum & 0xfe) >> 1))
                                  + pEntry->Filename[i]);
    }
    //construct slots and entry
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
//FIXME      pSlots[i].start=;
       memcpy (pSlots[i].name0_4, DirName + (nbSlots - i - 2) * 13, 10);
       memcpy (pSlots[i].name5_10, DirName + (nbSlots - i - 2) * 13 + 5, 12);
       memcpy (pSlots[i].name11_12, DirName + (nbSlots - i - 2) * 13 + 11, 4);
    }
  }
  //try to find nbSlots contiguous entries frees in directory
  if (!findDirSpace(DeviceExt, pDirFcb, nbSlots, &start))
  {
    vfatReleaseFCB(DeviceExt, pDirFcb);
    ExFreePool (Buffer);
    return STATUS_DISK_FULL;
  }

  if (RequestedOptions & FILE_DIRECTORY_FILE)
  {
    CurrentCluster = 0xffffffff;
    Status = NextCluster (DeviceExt, 0, &CurrentCluster, TRUE);
    if (CurrentCluster == 0xffffffff || !NT_SUCCESS(Status))
    {
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
      pEntry->FirstClusterHigh = CurrentCluster >> 16;
    }
    pEntry->FirstCluster = CurrentCluster;
  }

  size = DeviceExt->FatInfo.BytesPerCluster / sizeof(FATDirEntry);
  FileOffset.u.HighPart = 0;
  FileOffset.u.LowPart = start * sizeof(FATDirEntry);
  if (start / size == (start + nbSlots - 1) / size)
  {
    // one cluster
    CHECKPOINT;
    CcMapData (pDirFcb->FileObject, &FileOffset, nbSlots * sizeof(FATDirEntry),
               TRUE, &Context, (PVOID*)&pFatEntry);
    memcpy(pFatEntry, Buffer, nbSlots * sizeof(FATDirEntry));
  }
  else
  {
    // two clusters
    CHECKPOINT;
    size = DeviceExt->FatInfo.BytesPerCluster -
             (start * sizeof(FATDirEntry)) % DeviceExt->FatInfo.BytesPerCluster;
    CcMapData (pDirFcb->FileObject, &FileOffset, size, TRUE,
               &Context, (PVOID*)&pFatEntry);
    memcpy(pFatEntry, Buffer, size);
    CcSetDirtyPinnedData(Context, NULL);
    CcUnpinData(Context);
    FileOffset.u.LowPart += size;
    CcMapData (pDirFcb->FileObject, &FileOffset,
               nbSlots * sizeof(FATDirEntry) - size,
               TRUE, &Context, (PVOID*)&pFatEntry);
    memcpy(pFatEntry, (PVOID)Buffer + size, nbSlots * sizeof(FATDirEntry) - size);
  }
  CcSetDirtyPinnedData(Context, NULL);
  CcUnpinData(Context);

  // FIXME: check status
  vfatMakeFCBFromDirEntry (DeviceExt, pDirFcb, FileName, pEntry,
                           start, start + nbSlots - 1, &newFCB);
  vfatAttachFCBToFileObject (DeviceExt, newFCB, pFileObject);

  DPRINT ("new : entry=%11.11s\n", newFCB->entry.Filename);
  DPRINT ("new : entry=%11.11s\n", pEntry->Filename);

  if (RequestedOptions & FILE_DIRECTORY_FILE)
  {
    FileOffset.QuadPart = 0;
    CcMapData (pFileObject, &FileOffset, DeviceExt->FatInfo.BytesPerCluster, TRUE,
               &Context, (PVOID*)&pFatEntry);
    // clear the new directory cluster
    RtlZeroMemory (pFatEntry, DeviceExt->FatInfo.BytesPerCluster);
    // create '.' and '..'
    memcpy (&pFatEntry[0].Attrib, &pEntry->Attrib, sizeof(FATDirEntry) - 11);
    memcpy (pFatEntry[0].Filename, ".          ", 11);
    memcpy (&pFatEntry[1].Attrib, &pEntry->Attrib, sizeof(FATDirEntry) - 11);
    memcpy (pFatEntry[1].Filename, "..         ", 11);
    pFatEntry[1].FirstCluster = pDirFcb->entry.FirstCluster;
    pFatEntry[1].FirstClusterHigh = pDirFcb->entry.FirstClusterHigh;
    if (DeviceExt->FatInfo.FatType == FAT32)
    {
      if (pFatEntry[1].FirstCluster == (DeviceExt->FatInfo.RootCluster & 0xffff) &&
	  pFatEntry[1].FirstClusterHigh == (DeviceExt->FatInfo.RootCluster >> 16))
      {
        pFatEntry[1].FirstCluster = 0;
	pFatEntry[1].FirstClusterHigh = 0;
      }
    }
    else
    {
      if (pFatEntry[1].FirstCluster == 1)
      {
        pFatEntry[1].FirstCluster = 0;
      }
    }
    CcSetDirtyPinnedData(Context, NULL);
    CcUnpinData(Context);
  }
  vfatReleaseFCB (DeviceExt, pDirFcb);
  ExFreePool (Buffer);
  DPRINT ("addentry ok\n");
  return STATUS_SUCCESS;
}

NTSTATUS
delEntry (PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT pFileObject)
/*
 * deleting an existing FAT entry
 */
{
  VFATFCB Fcb;
  PVFATFCB pFcb = NULL, pDirFcb = NULL;
  NTSTATUS status;
  PWSTR pName;
  ULONG Entry = 0, startEntry, Read, CurrentCluster, NextCluster, i;
  FATDirEntry DirEntry;

  DPRINT ("delEntry PathFileName \'%S\'\n", pFileObject->FileName.Buffer);

  status = vfatGetFCBForFile(DeviceExt, &pDirFcb, &pFcb,
                             pFileObject->FileName.Buffer);
  if (pFcb != NULL)
  {
    vfatReleaseFCB(DeviceExt, pFcb);
  }
  if (!NT_SUCCESS(status))
  {
    if (pDirFcb != NULL)
    {
      vfatReleaseFCB(DeviceExt, pDirFcb);
    }
    return status;
  }
  pName = ((PVFATFCB)pFileObject->FsContext)->ObjectName;
  if (*pName == L'\\')
  {
    pName ++;
  }
  status = FindFile (DeviceExt, &Fcb, pDirFcb, pName, &Entry, &startEntry);

  if (NT_SUCCESS(status))
  {
    PVOID Context = NULL;
    LARGE_INTEGER Offset;
    FATDirEntry* pDirEntry;
    DPRINT ("delete entry: %d to %d\n", startEntry, Entry);
    Offset.u.HighPart = 0;
    for (i = startEntry; i <= Entry; i++)
    {
      if (Context == NULL || ((i * sizeof(FATDirEntry)) % PAGE_SIZE) == 0)
      {
        if (Context)
        {
          CcSetDirtyPinnedData(Context, NULL);
          CcUnpinData(Context);
        }
        Offset.u.LowPart = (i * sizeof(FATDirEntry) / PAGE_SIZE) * PAGE_SIZE;
        CcMapData (pDirFcb->FileObject, &Offset, PAGE_SIZE, TRUE,
                   &Context, (PVOID*)&pDirEntry);
      }
      pDirEntry[i % (PAGE_SIZE / sizeof(FATDirEntry))].Filename[0] = 0xe5;
      if (i == Entry)
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
      GetNextCluster (DeviceExt, CurrentCluster, &NextCluster, FALSE);
      // FIXME: check status
      WriteCluster(DeviceExt, CurrentCluster, 0);
      CurrentCluster = NextCluster;
    }
  }
  vfatReleaseFCB(DeviceExt, pDirFcb);
  return status;
}

/* EOF */
