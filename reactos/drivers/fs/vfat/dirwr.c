/* $Id: dirwr.c,v 1.16 2001/01/16 09:55:02 dwelch Exp $
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
  WCHAR DirName[MAX_PATH], *FileName, *PathFileName;
  VFATFCB FileFcb;
  ULONG Sector = 0, Entry = 0;
  PUCHAR Buffer;
  FATDirEntry *pEntries;
  NTSTATUS status;
  FILE_OBJECT FileObject;
  PVFATCCB pDirCcb;
  PVFATFCB pDirFcb, pFcb;
  short i, posCar, NameLen;

  PathFileName = pFileObject->FileName.Buffer;
  pFcb = ((PVFATCCB) pFileObject->FsContext2)->pFcb;
  DPRINT ("PathFileName \'%S\'\n", PathFileName);

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
  if (FileName[0] == 0 && DirName[0] == 0)
    return STATUS_SUCCESS;	//root : nothing to do ?
  memset (&FileObject, 0, sizeof (FILE_OBJECT));
  DPRINT ("open directory \'%S\' for update of entry \'%S\'\n", DirName,
	  FileName);
  status = VfatOpenFile (DeviceExt, &FileObject, DirName);
  if (!NT_SUCCESS (status))
    {
      DbgPrint ("Failed to open \'%S\'. Status %lx\n", DirName, status);
      return status;
    }
  pDirCcb = (PVFATCCB) FileObject.FsContext2;
  assert (pDirCcb);
  pDirFcb = pDirCcb->pFcb;
  assert (pDirFcb);
  FileFcb.ObjectName = &FileFcb.PathName[0];
  status = FindFile (DeviceExt, &FileFcb, pDirFcb, FileName, &Sector, &Entry);
  if (NT_SUCCESS (status))
    {
      Buffer = ExAllocatePool (NonPagedPool, BLOCKSIZE);
      DPRINT ("update entry: sector %d, entry %d\n", Sector, Entry);
      VfatReadSectors (DeviceExt->StorageDevice, Sector, 1, Buffer);
      pEntries = (FATDirEntry *) Buffer;
      memcpy (&pEntries[Entry], &pFcb->entry, sizeof (FATDirEntry));
      VfatWriteSectors (DeviceExt->StorageDevice, Sector, 1, Buffer);
      ExFreePool (Buffer);
    }
  VfatCloseFile (DeviceExt, &FileObject);
  return status;
}


NTSTATUS
addEntry (PDEVICE_EXTENSION DeviceExt,
	  PFILE_OBJECT pFileObject, ULONG RequestedOptions, UCHAR ReqAttr)
/*
  create a new FAT entry
*/
{
  WCHAR DirName[MAX_PATH], *FileName, *PathFileName;
  VFATFCB DirFcb, FileFcb;
  FATDirEntry FatEntry;
  NTSTATUS status;
  FILE_OBJECT FileObject;
  FATDirEntry *pEntry;
  slot *pSlots;
  ULONG LengthRead, Offset;
  short nbSlots = 0, nbFree = 0, i, j, posCar, NameLen;
  PUCHAR Buffer, Buffer2;
  BOOLEAN needTilde = FALSE, needLong = FALSE;
  PVFATFCB newFCB;
  PVFATCCB newCCB;
  ULONG CurrentCluster;
  KIRQL oldIrql;
  LARGE_INTEGER SystemTime, LocalTime;
  ULONG BytesPerCluster;
  NTSTATUS Status;

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
  memcpy (DirName, PathFileName, posCar * sizeof (WCHAR));
  DirName[posCar] = 0;
  // open parent directory
  memset (&FileObject, 0, sizeof (FILE_OBJECT));
  status = VfatOpenFile (DeviceExt, &FileObject, DirName);
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
  posCar = 0;
  for (i = 0; FileName[i]; i++)
    if (FileName[i] == '.')
      posCar = i;
  if (!posCar)
    posCar = i;
  if (posCar > 8)
    needTilde = TRUE;
  //copy 8 characters max
  memset (pEntry, ' ', 11);
  for (i = 0, j = 0; j < 8 && i < posCar; i++)
    {
      //FIXME : is there other characters to ignore ?
      if (FileName[i] != '.'
	  && FileName[i] != ' '
	  && FileName[i] != '+'
	  && FileName[i] != ','
	  && FileName[i] != ';'
	  && FileName[i] != '=' && FileName[i] != '[' && FileName[i] != ']')
	pEntry->Filename[j++] = toupper ((char) FileName[i]);
      else
	needTilde = TRUE;
    }
  //copy extension
  if (FileName[posCar])
    for (j = 0, i = posCar + 1; FileName[i] && i < posCar + 4; i++)
      {
	pEntry->Ext[j++] = toupper ((char) (FileName[i] & 0x7F));
      }
  if (FileName[i])
    needTilde = TRUE;
  //find good value for tilde
  if (needTilde)
    {
      needLong = TRUE;
      DPRINT ("searching a good value for tilde\n");
      for (i = 0; i < 6; i++)
	DirName[i] = pEntry->Filename[i];
      for (i = 0; i < 3; i++)
	DirName[i + 8] = pEntry->Ext[i];
      //try first with xxxxxx~y.zzz
      DirName[6] = '~';
      pEntry->Filename[6] = '~';
      DirName[8] = '.';
      DirName[12] = 0;
      for (i = 1; i < 9; i++)
	{
	  DirName[7] = '0' + i;
	  pEntry->Filename[7] = '0' + i;
	  status =
	    FindFile (DeviceExt, &FileFcb, &DirFcb, DirName, NULL, NULL);
	  if (status != STATUS_SUCCESS)
	    break;
	}
      //try second with xxxxx~yy.zzz
      if (i == 10)
	{
	  DirName[5] = '~';
	  for (; i < 99; i++)
	    {
	      DirName[7] = '0' + i;
	      pEntry->Filename[7] = '0' + i;
	      status =
		FindFile (DeviceExt, &FileFcb, &DirFcb, DirName, NULL, NULL);
	      if (status != STATUS_SUCCESS)
		break;
	    }
	}
      if (i == 100)		//FIXME : what to do after 99 tilde ?
	{
	  VfatCloseFile (DeviceExt, &FileObject);
	  ExFreePool (Buffer);
	  return STATUS_UNSUCCESSFUL;
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
	VfatReadFile (DeviceExt, &FileObject, &FatEntry, sizeof (FATDirEntry),
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
      if (IsDeletedEntry (&FatEntry, 0))
	nbFree++;
      else
	nbFree = 0;

      if (nbFree == nbSlots)
	break;
    }
  DPRINT ("nbSlots %d nbFree %d, entry number %d\n", nbSlots, nbFree, i);

  if (RequestedOptions & FILE_DIRECTORY_FILE)
    {
      NextCluster (DeviceExt, 0, &CurrentCluster, TRUE);
      // zero the cluster
      Buffer2 = ExAllocatePool (NonPagedPool, DeviceExt->BytesPerCluster);
      memset (Buffer2, 0, DeviceExt->BytesPerCluster);
      VfatRawWriteCluster (DeviceExt, 0, Buffer2, CurrentCluster);
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
	VfatWriteFile (DeviceExt, &FileObject, Buffer,
		      sizeof (FATDirEntry) * nbSlots, Offset, FALSE);
      DPRINT ("VfatWriteFile() returned: %x\n", status);
    }
  else
    {				//write at end of directory
      Offset = (i - nbFree) * sizeof (FATDirEntry);
      status =
	VfatWriteFile (DeviceExt, &FileObject, Buffer,
		      sizeof (FATDirEntry) * (nbSlots + 1), Offset, FALSE);
    }
  DPRINT ("write entry offset %d status=%x\n", Offset, status);
  newCCB = ExAllocatePool (NonPagedPool, sizeof (VFATCCB));
  newFCB = ExAllocatePool (NonPagedPool, sizeof (VFATFCB));
  memset (newCCB, 0, sizeof (VFATCCB));
  memset (newFCB, 0, sizeof (VFATFCB));
  newCCB->pFcb = newFCB;
  newCCB->PtrFileObject = pFileObject;
  newFCB->RefCount++;
  
  BytesPerCluster = DeviceExt->Boot->SectorsPerCluster * BLOCKSIZE;
  if (BytesPerCluster >= PAGESIZE)
    {
      Status = CcInitializeFileCache(pFileObject, &newFCB->RFCB.Bcb,
				     BytesPerCluster);
    }
  else
    {
      Status = CcInitializeFileCache(pFileObject, &newFCB->RFCB.Bcb, 
				     PAGESIZE);
    }

  /* 
   * FIXME : initialize all fields in FCB and CCB
   */
  KeAcquireSpinLock (&DeviceExt->FcbListLock, &oldIrql);
  InsertTailList (&DeviceExt->FcbListHead, &newFCB->FcbListEntry);
  KeReleaseSpinLock (&DeviceExt->FcbListLock, oldIrql);


  memcpy (&newFCB->entry, pEntry, sizeof (FATDirEntry));
  DPRINT ("new : entry=%11.11s\n", newFCB->entry.Filename);
  DPRINT ("new : entry=%11.11s\n", pEntry->Filename);
  vfat_wcsncpy (newFCB->PathName, PathFileName, MAX_PATH);
  newFCB->ObjectName = newFCB->PathName + (PathFileName - FileName);
  newFCB->pDevExt = DeviceExt;
  pFileObject->FsContext = (PVOID)&newFCB->RFCB;
  pFileObject->FsContext2 = newCCB;
  if (RequestedOptions & FILE_DIRECTORY_FILE)
    {
      // create . and ..
      memcpy (pEntry->Filename, ".          ", 11);
      status =
	VfatWriteFile (DeviceExt, pFileObject, pEntry, sizeof (FATDirEntry),
		      0L, FALSE);
      pEntry->FirstCluster =
	((VFATCCB *) (FileObject.FsContext2))->pFcb->entry.FirstCluster;
      pEntry->FirstClusterHigh =
	((VFATCCB *) (FileObject.FsContext2))->pFcb->entry.FirstClusterHigh;
      memcpy (pEntry->Filename, "..         ", 11);
      if (pEntry->FirstCluster == 1 && DeviceExt->FatType != FAT32)
	pEntry->FirstCluster = 0;
      status =
	VfatWriteFile (DeviceExt, pFileObject, pEntry, sizeof (FATDirEntry),
		      sizeof (FATDirEntry), FALSE);
    }
  VfatCloseFile (DeviceExt, &FileObject);
  ExFreePool (Buffer);
  DPRINT ("addentry ok\n");
  return STATUS_SUCCESS;
}

/* EOF */
