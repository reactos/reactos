/* $Id: dirwr.c,v 1.23 2002/01/08 00:49:01 dwelch Exp $
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

/*
 * Copies a file name into a directory slot (long file name entry)
 * and fills trailing slot space with 0xFFFF. This keeps scandisk
 * from complaining.
 */
static VOID
FillSlot (slot * Slot, WCHAR * FileName)
{
  BOOLEAN fill = FALSE;
  WCHAR *src = FileName;
  WCHAR *dst;
  int i;

  i = 5;
  dst = Slot->name0_4;
  while (i-- > 0)
    {
      if (fill == FALSE)
	*dst = *src;
      else
	*dst = 0xffff;

      if (fill == FALSE && (*src == 0))
	fill = TRUE;
      dst++;
      src++;
    }

  i = 6;
  dst = Slot->name5_10;
  while (i-- > 0)
    {
      if (fill == FALSE)
	*dst = *src;
      else
	*dst = 0xffff;

      if (fill == FALSE && (*src == 0))
	fill = TRUE;
      dst++;
      src++;
    }

  i = 2;
  dst = Slot->name11_12;
  while (i-- > 0)
    {
      if (fill == FALSE)
	*dst = *src;
      else
	*dst = 0xffff;

      if (fill == FALSE && (*src == 0))
	fill = TRUE;
      dst++;
      src++;
    }
}

NTSTATUS updEntry (PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT pFileObject)
/*
  update an existing FAT entry
*/
{
  PVOID Context;
  PVOID Buffer;
  NTSTATUS status;
  PVFATFCB pDirFcb = NULL, pFcb = NULL;
  LARGE_INTEGER Offset;

  DPRINT ("updEntry PathFileName \'%S\'\n", 
          ((PVFATCCB)(pFileObject->FsContext2))->pFcb->PathName);
  status = vfatGetFCBForFile(DeviceExt, &pDirFcb, &pFcb, 
             ((PVFATCCB)(pFileObject->FsContext2))->pFcb->PathName);
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

  Offset.QuadPart = pFcb->dirIndex * sizeof(FATDirEntry);
  if(CcMapData (pDirFcb->FileObject, &Offset, sizeof(FATDirEntry),
    TRUE, &Context, &Buffer))
  {
     memcpy(Buffer, &pFcb->entry, sizeof(FATDirEntry));
     CcSetDirtyPinnedData(Context, NULL);
     CcUnpinData(Context);
  }
  else
     DPRINT1 ("Failed write to \'%S\'.\n", pDirFcb->PathName);
   vfatReleaseFCB(DeviceExt, pDirFcb);
  return STATUS_SUCCESS;
}

NTSTATUS
addEntry (PDEVICE_EXTENSION DeviceExt,
	  PFILE_OBJECT pFileObject, ULONG RequestedOptions, UCHAR ReqAttr)
/*
  create a new FAT entry
*/
{
  WCHAR DirName[MAX_PATH], *FileName, *PathFileName;
  VFATFCB FileFcb;
  FATDirEntry FatEntry;
  NTSTATUS status;
  FATDirEntry *pEntry;
  slot *pSlots;
  ULONG LengthRead, Offset;
  short nbSlots = 0, nbFree = 0, i, j, posCar, NameLen;
  PUCHAR Buffer, Buffer2;
  BOOLEAN needTilde = FALSE, needLong = FALSE;
  PVFATFCB newFCB;
  ULONG CurrentCluster;
  LARGE_INTEGER SystemTime, LocalTime;
  NTSTATUS Status = STATUS_SUCCESS;
  PVFATFCB pDirFcb;

  PathFileName = pFileObject->FileName.Buffer;
  DPRINT ("addEntry: Pathname=%S\n", PathFileName);
  //find last \ in PathFileName
  posCar = -1;
  for (i = 0; PathFileName[i]; i++)
    if (PathFileName[i] == '\\')
      posCar = i;
  if (posCar == -1)
    return STATUS_UNSUCCESSFUL;
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
  Buffer =
    ExAllocatePool (NonPagedPool, (nbSlots + 1) * sizeof (FATDirEntry));
  memset (Buffer, 0, (nbSlots + 1) * sizeof (FATDirEntry));
  pEntry = (FATDirEntry *) (Buffer + (nbSlots - 1) * sizeof (FATDirEntry));
  pSlots = (slot *) Buffer;
  // create 8.3 name
  needTilde = FALSE;
  // find last point in name
  posCar = j = 0;
  for (i = 0; FileName[i]; i++)
    if (FileName[i] == '.')
    {
      posCar = i;
      if (i == j)
        j++;
    }
  if (!posCar)
    posCar = i;
  if (posCar < j)
  {
    posCar = i;
    needTilde = TRUE;
  }
  if (posCar > 8)
    needTilde = TRUE;
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
        needTilde = TRUE;
      else
        pEntry->Filename[j++] = toupper ((char) FileName[i]);
    }
  }
  //copy extension
  if (FileName[posCar])
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
          needTilde = TRUE;
        else
	        pEntry->Ext[j++] = toupper ((char) (FileName[i] & 0x7F));
      }
    }
  if (FileName[i])
    needTilde = TRUE;
  //find good value for tilde
  if (needTilde)
  {
    needLong = TRUE;
    DPRINT ("searching a good value for tilde\n");
    for (posCar = 0; posCar < 8 && pEntry->Filename[posCar] != ' '; posCar++);
    if (posCar == 0) // ??????????????????????
      pEntry->Filename[posCar++] = '_';
    posCar += 2;
    if (posCar > 8)
      posCar = 8;
    pEntry->Filename[posCar - 2] = '~';
    pEntry->Filename[posCar - 1] = '1';
    vfat8Dot3ToString (pEntry->Filename, pEntry->Ext, DirName);
    //try first with xxxxxx~y.zzz
    for (i = 1; i < 10; i++)
    {
      DirName[posCar-1] = '0' + i;
      pEntry->Filename[posCar - 1] = '0' + i;
	    status = FindFile (DeviceExt, &FileFcb, pDirFcb, DirName, NULL, NULL);
	    if (!NT_SUCCESS(status))
	      break;
    }
    if (i == 10)
    {
      posCar++;
      if (posCar > 8)
        posCar = 8;
      pEntry->Filename[posCar - 3] = '~';
      pEntry->Filename[posCar - 2] = '1';
      pEntry->Filename[posCar - 1] = '0';
      vfat8Dot3ToString (pEntry->Filename, pEntry->Ext, DirName);
      //try second with xxxxx~yy.zzz
      for (i = 10; i < 100; i++)
      {
        DirName[posCar - 1] = '0' + i % 10;
        DirName[posCar - 2] = '0' + i / 10;
        pEntry->Filename[posCar - 1] = '0' + i % 10;
        pEntry->Filename[posCar - 2] = '0' + i / 10;
	      status = FindFile (DeviceExt, &FileFcb, pDirFcb, DirName, NULL, NULL);
	      if (!NT_SUCCESS(status))
	        break;
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
    for (i = 0; i < posCar; i++)
	    if ((USHORT) pEntry->Filename[i] != FileName[i])
	    {
	      DPRINT ("i=%d,%d,%d\n", i, pEntry->Filename[i], FileName[i]);
	      needLong = TRUE;
	    }
    if (FileName[i])
	  {
	    i++;			//jump on point char
	    for (j = 0, i = posCar + 1; FileName[i] && i < posCar + 4; i++)
	    if ((USHORT) pEntry->Ext[j++] != FileName[i])
	    {
		    DPRINT ("i=%d,j=%d,%d,%d\n", i, j, pEntry->Filename[i],
		      FileName[i]);
        needLong = TRUE;
      }
    }
  }
  if (needLong == FALSE)
  {
    nbSlots = 1;
    memcpy (Buffer, pEntry, sizeof (FATDirEntry));
    memset (pEntry, 0, sizeof (FATDirEntry));
    pEntry = (FATDirEntry *) Buffer;
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
    pEntry->Attrib |= FILE_ATTRIBUTE_DIRECTORY;

  /* set dates and times */
  KeQuerySystemTime (&SystemTime);
  ExSystemTimeToLocalTime (&SystemTime, &LocalTime);
  FsdFileTimeToDosDateTime ((TIME *) & LocalTime,
			    &pEntry->CreationDate, &pEntry->CreationTime);
  pEntry->UpdateDate = pEntry->CreationDate;
  pEntry->UpdateTime = pEntry->CreationTime;
  pEntry->AccessDate = pEntry->CreationDate;

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
	pSlots[i].id = nbSlots - i - 1;
      else
	pSlots[i].id = nbSlots - i - 1 + 0x40;
      pSlots[i].alias_checksum = pSlots[0].alias_checksum;
//FIXME      pSlots[i].start=;
      FillSlot (&pSlots[i], FileName + (nbSlots - i - 2) * 13);
    }

  //try to find nbSlots contiguous entries frees in directory
  for (i = 0, status = STATUS_SUCCESS; status == STATUS_SUCCESS; i++)
    {
      status =
	VfatReadFile (DeviceExt, pDirFcb->FileObject, 
	   &FatEntry, sizeof (FATDirEntry), 
	   i * sizeof (FATDirEntry), &LengthRead, FALSE);
      if (status == STATUS_END_OF_FILE)
	break;
      if (!NT_SUCCESS (status))
	{
	  DPRINT1 ("VfatReadFile failed to read the directory entry\n");
	  break;
	}
      if (LengthRead != sizeof (FATDirEntry))
	{
	  DPRINT1 ("VfatReadFile did not read a complete directory entry\n");
	  break;
	}
    if (vfatIsDirEntryEndMarker(&FatEntry))
      break;
    if (vfatIsDirEntryDeleted(&FatEntry))
	nbFree++;
      else
	nbFree = 0;

      if (nbFree == nbSlots)
	break;
    }
  DPRINT ("nbSlots %d nbFree %d, entry number %d\n", nbSlots, nbFree, i);

  if (RequestedOptions & FILE_DIRECTORY_FILE)
    {
    CurrentCluster = 0xffffffff;
    status = NextCluster (DeviceExt, NULL, 0, &CurrentCluster, TRUE);
    if (CurrentCluster == 0xffffffff || !NT_SUCCESS(status))
    {
      vfatReleaseFCB(DeviceExt, pDirFcb);
      ExFreePool (Buffer);
      if (!NT_SUCCESS(status))
      {
        return status;
      }
      return STATUS_DISK_FULL;
    }
      // zero the cluster
      Buffer2 = ExAllocatePool (NonPagedPool, DeviceExt->BytesPerCluster);
      memset (Buffer2, 0, DeviceExt->BytesPerCluster);
      VfatRawWriteCluster (DeviceExt, 0, Buffer2, CurrentCluster, 1);
      ExFreePool (Buffer2);
      if (DeviceExt->FatType == FAT32)
	{
	  pEntry->FirstClusterHigh = CurrentCluster >> 16;
	  pEntry->FirstCluster = CurrentCluster;
	}
      else
	pEntry->FirstCluster = CurrentCluster;
    }
  if (nbFree == nbSlots)
    {				//use old slots
      Offset = (i - nbSlots + 1) * sizeof (FATDirEntry);
      status =
	VfatWriteFile (DeviceExt, pDirFcb->FileObject, 
	   Buffer, sizeof (FATDirEntry) * nbSlots, 
	   Offset, FALSE, FALSE);
    DPRINT ("VfatWriteFile() returned: %x\n", status);
    }
  else
    {				//write at end of directory
      Offset = (i - nbFree) * sizeof (FATDirEntry);
      status =
	VfatWriteFile (DeviceExt, pDirFcb->FileObject, 
	   Buffer, sizeof (FATDirEntry) * (nbSlots + 1), 
	   Offset, FALSE, FALSE);
    }
  DPRINT ("write entry offset %d status=%x\n", Offset, status);
  if (!NT_SUCCESS(status))
  {
    vfatReleaseFCB (DeviceExt, pDirFcb);
    if (RequestedOptions & FILE_DIRECTORY_FILE)
    {
      // free the reserved cluster
      WriteCluster(DeviceExt, CurrentCluster, 0);
    }
    ExFreePool (Buffer);
    return status;
  }

  // FEXME: check status
  vfatMakeFCBFromDirEntry (DeviceExt, pDirFcb, FileName, 
    pEntry, Offset / sizeof(FATDirEntry) + nbSlots-1, &newFCB);
  vfatAttachFCBToFileObject (DeviceExt, newFCB, pFileObject);

  DPRINT ("new : entry=%11.11s\n", newFCB->entry.Filename);
  DPRINT ("new : entry=%11.11s\n", pEntry->Filename);

  if (RequestedOptions & FILE_DIRECTORY_FILE)
    {
      // create . and ..
      memcpy (pEntry->Filename, ".          ", 11);
      status =
	VfatWriteFile (DeviceExt, pFileObject, pEntry, sizeof (FATDirEntry),
		      0L, FALSE, FALSE);
      pEntry->FirstCluster = pDirFcb->entry.FirstCluster;
      pEntry->FirstClusterHigh = pDirFcb->entry.FirstClusterHigh;
      memcpy (pEntry->Filename, "..         ", 11);
      if (pEntry->FirstCluster == 1 && DeviceExt->FatType != FAT32)
	pEntry->FirstCluster = 0;
      status =
	VfatWriteFile (DeviceExt, pFileObject, pEntry, sizeof (FATDirEntry),
		      sizeof (FATDirEntry), FALSE, FALSE);
    }
  vfatReleaseFCB (DeviceExt, pDirFcb);
  ExFreePool (Buffer);
  DPRINT ("addentry ok\n");
  return STATUS_SUCCESS;
}

NTSTATUS
delEntry (PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT pFileObject)
/*
  deleting an existing FAT entry
*/
{
  VFATFCB Fcb;
  PVFATFCB pFcb = NULL, pDirFcb = NULL;
  NTSTATUS status;
  PWSTR pName;
  ULONG Entry = 0, startEntry, Read, CurrentCluster, NextCluster, i;
  FATDirEntry DirEntry;

  DPRINT ("delEntry PathFileName \'%S\'\n", pFileObject->FileName.Buffer);

  status = vfatGetFCBForFile(DeviceExt, &pDirFcb, &pFcb, pFileObject->FileName.Buffer);
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
  pName = ((PVFATCCB)(pFileObject->FsContext2))->pFcb->ObjectName;
  if (*pName == L'\\')
  {
    pName ++;
  }
  status = FindFile (DeviceExt, &Fcb, pDirFcb, pName, &Entry, &startEntry);

  if (NT_SUCCESS(status))
  {
    DPRINT ("delete entry: %d to %d\n", startEntry, Entry);
    for (i = startEntry; i <= Entry; i++)
    {
      // FIXME: using Cc-functions
      VfatReadFile (DeviceExt, pDirFcb->FileObject, &DirEntry, 
        sizeof (FATDirEntry), i * sizeof(FATDirEntry), &Read, FALSE);
      DirEntry.Filename[0] = 0xe5;
      // FIXME: check status
      VfatWriteFile (DeviceExt, pDirFcb->FileObject, &DirEntry, 
        sizeof(FATDirEntry), i * sizeof(FATDirEntry), FALSE, FALSE);
    }
    CurrentCluster = vfatDirEntryGetFirstCluster (DeviceExt, &DirEntry);
    while (CurrentCluster && CurrentCluster != 0xffffffff)
    {
      GetNextCluster (DeviceExt, CurrentCluster, &NextCluster, FALSE);
      // FIXME: check status
      WriteCluster(DeviceExt, CurrentCluster, 0);
      CurrentCluster = NextCluster;
    }
  }
  return status;
}

/* EOF */
