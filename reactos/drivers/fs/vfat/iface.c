

/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/iface.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 * UPDATE HISTORY:
     ??           Created
     24-10-1998   Fixed bugs in long filename support
                  Fixed a bug that prevented unsuccessful file open requests being reported
                  Now works with long filenames that span over a sector boundary
     28-10-1998   Reads entire FAT into memory
                  VFatReadSector modified to read in more than one sector at a time
     7-11-1998    Fixed bug that assumed that directory data could be fragmented
     8-12-1998    Added FAT32 support
                  Added initial writability functions
                  WARNING: DO NOT ATTEMPT TO TEST WRITABILITY FUNCTIONS!!!
     12-12-1998   Added basic support for FILE_STANDARD_INFORMATION request

*/

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/string.h>
#include <wstring.h>
#include <ddk/cctypes.h>

#define NDEBUG
#include <internal/debug.h>

#include "vfat.h"

//#include "dbgpool.c"

/* GLOBALS *****************************************************************/

static PDRIVER_OBJECT VFATDriverObject;
PVfatFCB pFirstFcb;

/* FUNCTIONS ****************************************************************/

ULONG Fat32GetNextCluster(PDEVICE_EXTENSION DeviceExt, ULONG CurrentCluster)
/*
 * FUNCTION: Retrieve the next FAT32 cluster from the FAT table via a physical
 *           disk read
 */
{
   ULONG FATsector;
   ULONG FATeis;
   PULONG Block;
   
   Block = ExAllocatePool(NonPagedPool,1024);
   FATsector=CurrentCluster/(512/sizeof(ULONG));
   FATeis=CurrentCluster-(FATsector*(512/sizeof(ULONG)));
   VFATReadSectors(DeviceExt->StorageDevice
        ,(ULONG)(DeviceExt->FATStart+FATsector), 1,(UCHAR*) Block);
   CurrentCluster = Block[FATeis];
   if (CurrentCluster >= 0xffffff8 && CurrentCluster <= 0xfffffff)
    CurrentCluster = 0xffffffff;
   ExFreePool(Block);
   return(CurrentCluster);
}

ULONG Fat16GetNextCluster(PDEVICE_EXTENSION DeviceExt, ULONG CurrentCluster)
/*
 * FUNCTION: Retrieve the next FAT16 cluster from the FAT table from the
 *           in-memory FAT
 */
{
   PUSHORT Block;
   Block=(PUSHORT)DeviceExt->FAT;
   CurrentCluster = Block[CurrentCluster];
   if (CurrentCluster >= 0xfff8 && CurrentCluster <= 0xffff)
     CurrentCluster = 0xffffffff;
   DPRINT("Returning %x\n",CurrentCluster);
   return(CurrentCluster);
}

ULONG Fat12GetNextCluster(PDEVICE_EXTENSION DeviceExt, ULONG CurrentCluster)
/*
 * FUNCTION: Retrieve the next FAT12 cluster from the FAT table from the
 *           in-memory FAT
 */
{
 unsigned char* CBlock;
 ULONG FATOffset;
 ULONG Entry;
   CBlock = DeviceExt->FAT;
   FATOffset = (CurrentCluster * 12)/ 8;//first byte containing value
   if ((CurrentCluster % 2) == 0)
   {
    Entry = CBlock[FATOffset];
    Entry |= ((CBlock[FATOffset+1] & 0xf)<<8);
   }
   else
   {
    Entry = (CBlock[FATOffset] >> 4);
    Entry |= (CBlock[FATOffset+1] << 4);
   }
   DPRINT("Entry %x\n",Entry);
   if (Entry >= 0xff8 && Entry <= 0xfff)
    Entry = 0xffffffff;
   DPRINT("Returning %x\n",Entry);
   return(Entry);
}

ULONG GetNextCluster(PDEVICE_EXTENSION DeviceExt, ULONG CurrentCluster)
/*
 * FUNCTION: Retrieve the next cluster depending on the FAT type
 */
{
   
   DPRINT("GetNextCluster(DeviceExt %x, CurrentCluster %x)\n",
	    DeviceExt,CurrentCluster);
   if (DeviceExt->FatType == FAT16)
	return(Fat16GetNextCluster(DeviceExt, CurrentCluster));
   else if (DeviceExt->FatType == FAT32)
	return(Fat32GetNextCluster(DeviceExt, CurrentCluster));
   else
	return(Fat12GetNextCluster(DeviceExt, CurrentCluster));
}

ULONG FAT16FindAvailableCluster(PDEVICE_EXTENSION DeviceExt)
/*
 * FUNCTION: Finds the first available cluster in a FAT16 table
 */
{
 PUSHORT Block;
 int i;
   Block=(PUSHORT)DeviceExt->FAT;
   for(i=2;i<(DeviceExt->Boot->FATSectors*256) ;i++)
     if(Block[i]==0)
       return (i);
   /* Give an error message (out of disk space) if we reach here) */
   return 0;
}

ULONG FAT12FindAvailableCluster(PDEVICE_EXTENSION DeviceExt)
/*
 * FUNCTION: Finds the first available cluster in a FAT12 table
 */
{
 ULONG FATOffset;
 ULONG Entry;
 PUCHAR CBlock=DeviceExt->FAT;
 ULONG i;
   for(i=2;i<((DeviceExt->Boot->FATSectors*512*8)/12) ;i++)
   {
     FATOffset = (i * 12)/8;
     if ((i % 2) == 0)
     {
       Entry = CBlock[FATOffset];
       Entry |= ((CBlock[FATOffset + 1] & 0xf)<<8);
     }
     else
     {
       Entry = (CBlock[FATOffset] >> 4);
       Entry |= (CBlock[FATOffset + 1] << 4);
     }
     if(Entry==0)
         return (i);
   }
   /* Give an error message (out of disk space) if we reach here) */
   DbgPrint("Disk full, %d clusters used\n",i);
   return 0;
}

ULONG FAT32FindAvailableCluster(PDEVICE_EXTENSION DeviceExt)
/*
 * FUNCTION: Finds the first available cluster in a FAT32 table
 */
{
 ULONG sector;
 PULONG Block;
 int i;
   Block = ExAllocatePool(NonPagedPool,BLOCKSIZE);
   for(sector=0
       ;sector<  ((struct _BootSector32*)(DeviceExt->Boot))->FATSectors32
       ;sector++)
   {
     VFATReadSectors(DeviceExt->StorageDevice
        ,(ULONG)(DeviceExt->FATStart+sector), 1,(UCHAR*) Block);

     for(i=0; i<512; i++)
     {
       if(Block[i]==0)
       {
         ExFreePool(Block);
         return (i+sector*128);
       }
     }
   }
   /* Give an error message (out of disk space) if we reach here) */
   ExFreePool(Block);
   return 0;
}

void  FAT12WriteCluster(PDEVICE_EXTENSION DeviceExt, ULONG ClusterToWrite,
                        ULONG NewValue)
/*
 * FUNCTION: Writes a cluster to the FAT12 physical and in-memory tables
 */
{
 ULONG FATsector;
 ULONG FATOffset;
 PUCHAR CBlock=DeviceExt->FAT;
 int i;
   FATOffset = (ClusterToWrite * 12)/8;
   if ((ClusterToWrite % 2) == 0)
   {
     CBlock[FATOffset]=NewValue;
     CBlock[FATOffset + 1] &=0xf0;
     CBlock[FATOffset + 1]
         |= (NewValue&0xf00)>>8;
   }
   else
   {
     CBlock[FATOffset] &=0x0f;
     CBlock[FATOffset]
         |= (NewValue&0xf)<<4;
     CBlock[FATOffset+1]=NewValue>>4;
   }
   /* Write the changed FAT sector(s) to disk */
   FATsector=FATOffset/BLOCKSIZE;
   for(i=0;i<DeviceExt->Boot->FATCount;i++)
   {
      if( (FATOffset%BLOCKSIZE)==(BLOCKSIZE-1))//entry is on 2 sectors
      {
        VFATWriteSectors(DeviceExt->StorageDevice,
                   DeviceExt->FATStart+FATsector
                   +i*DeviceExt->Boot->FATSectors,
                       2,
                   CBlock+FATsector*512);
      }
      else
      {
        VFATWriteSectors(DeviceExt->StorageDevice,
                   DeviceExt->FATStart+FATsector
                   +i*DeviceExt->Boot->FATSectors,
                       1,
                   CBlock+FATsector*512);
      }
   }
}

void  FAT16WriteCluster(PDEVICE_EXTENSION DeviceExt, ULONG ClusterToWrite,
                        ULONG NewValue)
/*
 * FUNCTION: Writes a cluster to the FAT16 physical and in-memory tables
 */
{
 ULONG FATsector;
 PUSHORT Block;
DbgPrint("FAT16WriteCluster %u : %u\n",ClusterToWrite,NewValue);
   Block=(PUSHORT)DeviceExt->FAT;
   FATsector=ClusterToWrite/(512/sizeof(USHORT));

   /* Update the in-memory FAT */
   Block[ClusterToWrite] = NewValue;
   /* Write the changed FAT sector to disk */
   VFATWriteSectors(DeviceExt->StorageDevice,
 	            DeviceExt->FATStart+FATsector,
                    1,
	            (UCHAR *)Block);
}

void  FAT32WriteCluster(PDEVICE_EXTENSION DeviceExt, ULONG ClusterToWrite,
                        ULONG NewValue)
/*
 * FUNCTION: Writes a cluster to the FAT32 physical tables
 */
{
 ULONG FATsector;
 ULONG FATeis;
 PUSHORT Block;
DbgPrint("FAT32WriteCluster %u : %u\n",ClusterToWrite,NewValue);
   Block = ExAllocatePool(NonPagedPool,BLOCKSIZE);
   FATsector=ClusterToWrite/128;
   FATeis=ClusterToWrite-(FATsector*128);
   /* load sector, change value, then rewrite sector */
   VFATReadSectors(DeviceExt->StorageDevice,
 	            DeviceExt->FATStart+FATsector,
                    1,
	            (UCHAR *)Block);
   Block[FATeis] = NewValue;
   VFATWriteSectors(DeviceExt->StorageDevice,
 	            DeviceExt->FATStart+FATsector,
                    1,
	            (UCHAR *)Block);
   ExFreePool(Block);
}

void  WriteCluster(PDEVICE_EXTENSION DeviceExt, ULONG ClusterToWrite,
                   ULONG NewValue)
/*
 * FUNCTION: Write a changed FAT entry
 */
{
   if(DeviceExt->FatType==FAT16)
     FAT16WriteCluster(DeviceExt, ClusterToWrite, NewValue);
   else if(DeviceExt->FatType==FAT32)
     FAT32WriteCluster(DeviceExt, ClusterToWrite, NewValue);
   else
     FAT12WriteCluster(DeviceExt, ClusterToWrite, NewValue);
}

ULONG GetNextWriteCluster(PDEVICE_EXTENSION DeviceExt, ULONG CurrentCluster)
/*
 * FUNCTION: Determines the next cluster to be written
 */
{
 ULONG LastCluster, NewCluster;
   DPRINT("GetNextWriteCluster(DeviceExt %x, CurrentCluster %x)\n",
	    DeviceExt,CurrentCluster);

   /* Find out what was happening in the last cluster's AU */
   LastCluster=GetNextCluster(DeviceExt,CurrentCluster);
   /* Check to see if we must append or overwrite */
   if (LastCluster==0xffffffff)
   {//we are after last existing cluster : we must add one to file
        /* Append */
        /* Firstly, find the next available open allocation unit */
        if(DeviceExt->FatType == FAT16)
           NewCluster = FAT16FindAvailableCluster(DeviceExt);
        else if(DeviceExt->FatType == FAT32)
           NewCluster = FAT32FindAvailableCluster(DeviceExt);
        else
           NewCluster = FAT12FindAvailableCluster(DeviceExt);
        /* Mark the new AU as the EOF */
        WriteCluster(DeviceExt, NewCluster, 0xFFFFFFFF);
        /* Now, write the AU of the LastCluster with the value of the newly
           found AU */
        if(CurrentCluster)
          WriteCluster(DeviceExt, CurrentCluster, NewCluster);
        /* Return NewCluster as CurrentCluster */
        return NewCluster;
   }
   else
   {
        /* Overwrite: Return LastCluster as CurrentCluster */
        return LastCluster;
   }
}

ULONG ClusterToSector(PDEVICE_EXTENSION DeviceExt,
			      unsigned long Cluster)
/*
 * FUNCTION: Converts the cluster number to a sector number for this physical
 *           device
 */
{
  return DeviceExt->dataStart+((Cluster-2)*DeviceExt->Boot->SectorsPerCluster);
}

void RtlAnsiToUnicode(PWSTR Dest, PCH Source, ULONG Length)
/*
 * FUNCTION: Convert an ANSI string to it's Unicode equivalent
 */
{
   int i;
   
   for (i=0; (i<Length && Source[i] != ' '); i++)
     {
	Dest[i] = Source[i];
     }
   Dest[i]=0;
}

void RtlCatAnsiToUnicode(PWSTR Dest, PCH Source, ULONG Length)
/*
 * FUNCTION: Appends a converted ANSI to Unicode string to the end of an
 *           existing Unicode string
 */
{
   ULONG i;
   
   while((*Dest)!=0)
     {
	Dest++;
     }
   for (i=0; (i<Length && Source[i] != ' '); i++)
     {
	Dest[i] = Source[i];
     }
   Dest[i]=0;
}

void vfat_initstr(wchar_t *wstr, ULONG wsize)
/*
 * FUNCTION: Initialize a string for use with a long file name
 */
{
  int i;
  wchar_t nc=0;
  for(i=0; i<wsize; i++)
  {
    *wstr=nc;
    wstr++;
  }
  wstr=wstr-wsize;
}

wchar_t * vfat_wcsncat(wchar_t * dest, const wchar_t * src,size_t wstart, size_t wcount)
/*
 * FUNCTION: Append a string for use with a long file name
 */
{
   int i;

   dest+=wstart;
   for(i=0; i<wcount; i++)
   {
     *dest=src[i];
     dest++;
   }
   dest=dest-(wcount+wstart);

   return dest;
}

wchar_t * vfat_wcsncpy(wchar_t * dest, const wchar_t *src,size_t wcount)
/*
 * FUNCTION: Copy a string for use with long file names
 */
{
 int i;
   
   for (i=0;i<wcount;i++)
   {
     dest[i]=src[i];
     if(!dest[i]) break;
   }
   return(dest);
}

wchar_t * vfat_movstr(const wchar_t *src, ULONG dpos,
                      ULONG spos, ULONG len)
/*
 * FUNCTION: Move the characters in a string to a new position in the same
 *           string
 */
{
 int i;

  if(dpos<=spos)
  {
    for(i=0; i<len; i++)
    {
      src[dpos++]=src[spos++];
    }
  }
  else
  {
    dpos+=len-1;
    spos+=len-1;
    for(i=0; i<len; i++)
    {
      src[dpos--]=src[spos--];
    }
  }

  return(src);
}

BOOLEAN IsLastEntry(PVOID Block, ULONG Offset)
/*
 * FUNCTION: Determine if the given directory entry is the last
 */
{
   return(((FATDirEntry *)Block)[Offset].Filename[0] == 0);
}

BOOLEAN IsVolEntry(PVOID Block, ULONG Offset)
/*
 * FUNCTION: Determine if the given directory entry is a vol entry
 */
{
   if( (((FATDirEntry *)Block)[Offset].Attrib)==0x28 ) return TRUE;
   else return FALSE;
}

BOOLEAN IsDeletedEntry(PVOID Block, ULONG Offset)
/*
 * FUNCTION: Determines if the given entry is a deleted one
 */
{
   /* Checks special character */

   return ((((FATDirEntry *)Block)[Offset].Filename[0] == 0xe5));
}

BOOLEAN GetEntryName(PVOID Block, PULONG _Offset, PWSTR Name, PULONG _jloop,
  PDEVICE_EXTENSION DeviceExt, ULONG * _StartingSector)
/*
 * FUNCTION: Retrieves the file name, be it in short or long file name format
 */
{
   FATDirEntry* test;
   slot* test2;
   ULONG Offset = *_Offset;
   ULONG StartingSector = *_StartingSector;
   ULONG jloop = *_jloop;
   ULONG cpos;
   WCHAR tmp[256];
   
   test = (FATDirEntry *)Block;
   test2 = (slot *)Block;
   
   *Name = 0;

   if (IsDeletedEntry(Block,Offset))
     {
	return(FALSE);
     }
   
   if(test2[Offset].attr == 0x0f) 
     {
        vfat_initstr(Name, 256);
	vfat_wcsncpy(Name,test2[Offset].name0_4,5);
	vfat_wcsncat(Name,test2[Offset].name5_10,5,6);
	vfat_wcsncat(Name,test2[Offset].name11_12,11,2);

        cpos=0;
        while((test2[Offset].id!=0x41) && (test2[Offset].id!=0x01) &&
	      (test2[Offset].attr>0)) 
	  {
	     Offset++;
             if(Offset==ENTRIES_PER_SECTOR) {
               Offset=0;
               StartingSector++;//FIXME : nor always the next sector
               jloop++;
               VFATReadSectors(DeviceExt->StorageDevice,StartingSector,1,Block);
               test2 = (slot *)Block;
             }
             cpos++;
             vfat_movstr(Name, 13, 0, cpos*13);
             vfat_wcsncpy(Name, test2[Offset].name0_4, 5);
             vfat_wcsncat(Name,test2[Offset].name5_10,5,6);
             vfat_wcsncat(Name,test2[Offset].name11_12,11,2);

          }

	if (IsDeletedEntry(Block,Offset+1))
	  {
	     Offset++;
	     *_Offset = Offset;
             *_jloop = jloop;
             *_StartingSector = StartingSector;
	     return(FALSE);
	  }
	
	*_Offset = Offset;
        *_jloop = jloop;
        *_StartingSector = StartingSector;
	
	return(TRUE);
     }   
      
   RtlAnsiToUnicode(Name,test[Offset].Filename,8);
   if (test[Offset].Ext[0]!=' ')
     {
	RtlCatAnsiToUnicode(Name,".",1);
     }
   RtlCatAnsiToUnicode(Name,test[Offset].Ext,3);
      
   *_Offset = Offset;
   
   return(TRUE);
}

BOOLEAN wstrcmpi(PWSTR s1, PWSTR s2)
/*
 * FUNCTION: Compare to wide character strings
 * return TRUE if s1==s2
 */
{
   while (wtolower(*s1)==wtolower(*s2))
     {
	if ((*s1)==0 && (*s2)==0)
	  {
	     return(TRUE);
	  }
	
	s1++;
	s2++;	
     }
   return(FALSE);
}
BOOLEAN wstrcmpjoki(PWSTR s1, PWSTR s2)
/*
 * FUNCTION: Compare to wide character strings, s2 with jokers (* or ?)
 * return TRUE if s1 like s2
 */
{
   while ((*s2=='?')||(wtolower(*s1)==wtolower(*s2)))
   {
      if ((*s1)==0 && (*s2)==0)
        return(TRUE);
      s1++;
      s2++;	
   }
   if(*s2=='*')
   {
     s2++;
     while (*s1)
       if (wstrcmpjoki(s1,s2)) return TRUE;
       else s1++;
   }
   if ((*s1)==0 && (*s2)==0)
        return(TRUE);
   return(FALSE);
}

NTSTATUS FindFile(PDEVICE_EXTENSION DeviceExt, PVfatFCB Fcb,
          PVfatFCB Parent, PWSTR FileToFind,ULONG *StartSector,ULONG *Entry)
/*
 * FUNCTION: Find a file
 */
{
 ULONG i, j;
 ULONG Size;
 char* block;
 WCHAR name[256];
 ULONG StartingSector;
 ULONG NextCluster;
   WCHAR TempStr[2];
   
   DPRINT("FindFile(Parent %x, FileToFind '%w')\n",Parent,FileToFind);
   
   if (wcslen(FileToFind)==0)
     {
	TempStr[0] = (WCHAR)'.';
	TempStr[1] = 0;
	FileToFind=&TempStr;
     }
   if (Parent != NULL)
     {
	DPRINT("Parent->entry.FirstCluster %d\n",Parent->entry.FirstCluster);
     }
   
   if (Parent == NULL||Parent->entry.FirstCluster==1)
   {
     Size = DeviceExt->rootDirectorySectors;//FIXME : in fat32, no limit
     StartingSector = DeviceExt->rootStart;
     NextCluster=0;
     if(FileToFind[0]==0 ||(FileToFind[0]=='\\' && FileToFind[1]==0) ||
	(FileToFind[0]=='.' && FileToFind[1]==0))
     {// it's root : complete essentials fields then return ok
       memset(Fcb,0,sizeof(VfatFCB));
       memset(Fcb->entry.Filename,' ',11);
       Fcb->entry.FileSize=DeviceExt->rootDirectorySectors*BLOCKSIZE;
       Fcb->entry.Attrib=FILE_ATTRIBUTE_DIRECTORY;
       if (DeviceExt->FatType == FAT32)
         Fcb->entry.FirstCluster=2;
       else Fcb->entry.FirstCluster=1;//FIXME : is 1 the good value for mark root?
       if(StartSector) *StartSector=StartingSector;
       if(Entry) *Entry=0;
       return(STATUS_SUCCESS);
     }
   }
   else
   {
     DPRINT("Parent->entry.FileSize %x\n",Parent->entry.FileSize);
	
     Size = ULONG_MAX;
     if (DeviceExt->FatType == FAT32)
       NextCluster = Parent->entry.FirstCluster
               +Parent->entry.FirstClusterHigh*65536;
     else
       NextCluster = Parent->entry.FirstCluster;
     StartingSector = ClusterToSector(DeviceExt, NextCluster);
     if(Parent->entry.FirstCluster==1 && DeviceExt->FatType!=FAT32)
     {// read of root directory in FAT16 or FAT12
       StartingSector=DeviceExt->rootStart;
     }
   }
   block = ExAllocatePool(NonPagedPool,BLOCKSIZE);
   if (StartSector && (*StartSector)) StartingSector=*StartSector;
   i=(Entry)?(*Entry):0;
   DPRINT("FindFile : start at sector %lx, entry %ld\n",StartingSector,i);
   for (j=0; j<Size; j++)
   {
     VFATReadSectors(DeviceExt->StorageDevice,StartingSector,1,block);

     for (i=(Entry)?(*Entry):0; i<ENTRIES_PER_SECTOR; i++)
     {
       if (IsVolEntry((PVOID)block,i))
          continue;
       if (IsLastEntry((PVOID)block,i))
       {
         ExFreePool(block);
         if(StartSector) *StartSector=StartingSector;
         if(Entry) *Entry=i;
         return(STATUS_UNSUCCESSFUL);
       }
       if (GetEntryName((PVOID)block,&i,name,&j,DeviceExt,&StartingSector))
       {
	 DPRINT("Comparing '%w' '%w'\n",name,FileToFind);
         if (wstrcmpjoki(name,FileToFind))
         {
           /* In the case of a long filename, the firstcluster is stored in
                         the next record -- where it's short name is */
           if(((FATDirEntry *)block)[i].Attrib==0x0f) i++;
           if( i==(ENTRIES_PER_SECTOR))
           {// entry is in next sector
             StartingSector++;
             //FIXME : treat case of next sector fragmented
             VFATReadSectors(DeviceExt->StorageDevice,StartingSector,1,block);
             i=0;
           }
           memcpy(&Fcb->entry,&((FATDirEntry *)block)[i],
                          sizeof(FATDirEntry));
           vfat_wcsncpy(Fcb->ObjectName,name,MAX_PATH);
           ExFreePool(block);
           if(StartSector) *StartSector=StartingSector;
           if(Entry) *Entry=i;
           return(STATUS_SUCCESS);
         }
       }
     }
     // not found in this sector, try next :

     /* directory can be fragmented although it is best to keep them
        unfragmented */
     if(Entry) *Entry=0;
     StartingSector++;
     if ((Parent != NULL && Parent->entry.FirstCluster!=1)
           || DeviceExt->FatType ==FAT32)
     {
       if(StartingSector==ClusterToSector(DeviceExt,NextCluster+1))
       {
         NextCluster = GetNextCluster(DeviceExt,NextCluster);
         if (NextCluster == 0||NextCluster==0xffffffff)
         {
           if(StartSector) *StartSector=StartingSector;
           if(Entry) *Entry=i;
           ExFreePool(block);
           return(STATUS_UNSUCCESSFUL);
         }
         StartingSector = ClusterToSector(DeviceExt,NextCluster);
       }
     }
   }
   ExFreePool(block);
   if(StartSector) *StartSector=StartingSector;
   if(Entry) *Entry=i;
   return(STATUS_UNSUCCESSFUL);
}


NTSTATUS FsdCloseFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject)
/*
 * FUNCTION: Closes a file
 */
{
 PVfatFCB pFcb;
 PVfatCCB pCcb;
 //FIXME : update entry in directory ?
   pCcb = (PVfatCCB)(FileObject->FsContext2);
   pFcb = pCcb->pFcb;
   pFcb->RefCount--;
   if(pFcb->RefCount<=0)
   {
     if(pFcb->prevFcb)
       pFcb->prevFcb->nextFcb=pFcb->nextFcb;
     else
       pFirstFcb=pFcb->nextFcb;
     if(pFcb->nextFcb)
       pFcb->nextFcb->prevFcb=pFcb->prevFcb;
     ExFreePool(pFcb);
   }
   ExFreePool(pCcb);
   return STATUS_SUCCESS;
}

NTSTATUS FsdOpenFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject, 
		     PWSTR FileName)
/*
 * FUNCTION: Opens a file
 */
{
 PWSTR current;
 PWSTR next;
 PWSTR string;
 PVfatFCB ParentFcb;
 PVfatFCB Fcb,pRelFcb;
 PVfatFCB Temp;
 PVfatCCB newCCB,pRelCcb;
 NTSTATUS Status;
 PFILE_OBJECT pRelFileObject;
 PWSTR AbsFileName=NULL;
 short i,j;

   DPRINT("FsdOpenFile(%08lx, %08lx, %w)\n", 
          DeviceExt,
          FileObject,
          FileName);
   
  //FIXME : treat relative name
   if(FileObject->RelatedFileObject)
   {
DbgPrint("try related for %w\n",FileName);
     pRelFileObject=FileObject->RelatedFileObject;
     pRelCcb=pRelFileObject->FsContext2;
     assert(pRelCcb);
     pRelFcb=pRelCcb->pFcb;
     assert(pRelFcb);
     // verify related object is a directory and target name don't start with \.
     if( !(pRelFcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY)
         || (FileName[0]!= '\\') )
     {
       Status=STATUS_INVALID_PARAMETER;
       return Status;
     }
     // construct absolute path name
     AbsFileName=ExAllocatePool(NonPagedPool,MAX_PATH);
     for (i=0;pRelFcb->PathName[i];i++)
       AbsFileName[i]=pRelFcb->PathName[i];
     AbsFileName[i++]='\\';
     for (j=0;FileName[j]&&i<MAX_PATH;j++)
       AbsFileName[i++]=FileName[j];
     assert(i<MAX_PATH);
     AbsFileName[i]=0;
     FileName=AbsFileName;
   }
   // try first to find an existing FCB in memory
   for (Fcb=pFirstFcb;Fcb; Fcb=Fcb->nextFcb)
   {
     if (DeviceExt==Fcb->pDevExt
          && wstrcmpi(FileName,Fcb->PathName))
     {
        Fcb->RefCount++;
        FileObject->FsContext =(PVOID) &Fcb->NTRequiredFCB;
        newCCB = ExAllocatePool(NonPagedPool,sizeof(VfatCCB));
        memset(newCCB,0,sizeof(VfatCCB));
        FileObject->FsContext2 = newCCB;
        newCCB->pFcb=Fcb;
        newCCB->PtrFileObject=FileObject;
        if(AbsFileName)ExFreePool(AbsFileName);
        return(STATUS_SUCCESS);
     }
   }
   string = FileName;
   ParentFcb = NULL;
   Fcb = ExAllocatePool(NonPagedPool, sizeof(VfatFCB));
   memset(Fcb,0,sizeof(VfatFCB));
   Fcb->ObjectName=Fcb->PathName;
   next = &string[0];   

   while (next!=NULL)
   {
     *next = '\\';
     current = next+1;
     next = wcschr(next+1,'\\');
     if (next!=NULL)
	{
	   *next=0;
	}
      DPRINT("current '%w'\n",current);
      Status = FindFile(DeviceExt,Fcb,ParentFcb,current,NULL,NULL);
      if (Status != STATUS_SUCCESS)
	{
	   if (Fcb != NULL)
	     ExFreePool(Fcb);
	   if (ParentFcb != NULL)
	     ExFreePool(ParentFcb);
	   if(AbsFileName)ExFreePool(AbsFileName);
		return(Status);
	}
      Temp = Fcb;
      if (ParentFcb == NULL)
	{
	   Fcb = ExAllocatePool(NonPagedPool,sizeof(VfatFCB));
	   memset(Fcb,0,sizeof(VfatFCB));
	   Fcb->ObjectName=Fcb->PathName;
	}
      else Fcb = ParentFcb;
      ParentFcb = Temp;
   }
 FileObject->FsContext =(PVOID) &ParentFcb->NTRequiredFCB;
 newCCB = ExAllocatePool(NonPagedPool,sizeof(VfatCCB));
 memset(newCCB,0,sizeof(VfatCCB));
 FileObject->FsContext2 = newCCB;
 newCCB->pFcb=ParentFcb;
 newCCB->PtrFileObject=FileObject;
 ParentFcb->RefCount++;
 //FIXME : initialize all fields in FCB and CCB
 ParentFcb->nextFcb=pFirstFcb;
 pFirstFcb=ParentFcb;
   vfat_wcsncpy(ParentFcb->PathName,FileName,MAX_PATH);
 ParentFcb->ObjectName=ParentFcb->PathName+(current-FileName);
 ParentFcb->pDevExt=DeviceExt;
 DPRINT("file open, fcb=%x\n",ParentFcb);
 DPRINT("FileSize %d\n",ParentFcb->entry.FileSize);
 if(Fcb) ExFreePool(Fcb);
 if(AbsFileName)ExFreePool(AbsFileName);
   return(STATUS_SUCCESS);
 }
 
 
 BOOLEAN FsdHasFileSystem(PDEVICE_OBJECT DeviceToMount)
/*
 * FUNCTION: Tests if the device contains a filesystem that can be mounted 
 *           by this fsd
 */
{
   BootSector* Boot;

   Boot = ExAllocatePool(NonPagedPool,512);

   VFATReadSectors(DeviceToMount, 0, 1, (UCHAR *)Boot);

   if (strncmp(Boot->SysType,"FAT12",5)==0 ||
       strncmp(Boot->SysType,"FAT16",5)==0 ||
       strncmp(((struct _BootSector32 *)(Boot))->SysType,"FAT32",5)==0)
     {
	ExFreePool(Boot);
	return(TRUE);
     }
   ExFreePool(Boot);
   return(FALSE);
}

NTSTATUS FsdMountDevice(PDEVICE_EXTENSION DeviceExt,
			PDEVICE_OBJECT DeviceToMount)
/*
 * FUNCTION: Mounts the device
 */
{
   DPRINT("Mounting VFAT device...");
   DPRINT("DeviceExt %x\n",DeviceExt);

   DeviceExt->Boot = ExAllocatePool(NonPagedPool,512);
   VFATReadSectors(DeviceToMount, 0, 1, (UCHAR *)DeviceExt->Boot);
   
   DPRINT("DeviceExt->Boot->BytesPerSector %x\n",
	  DeviceExt->Boot->BytesPerSector);
   
   DeviceExt->FATStart=DeviceExt->Boot->ReservedSectors;
   DeviceExt->rootDirectorySectors=
     (DeviceExt->Boot->RootEntries*32)/DeviceExt->Boot->BytesPerSector;
   DeviceExt->rootStart=
     DeviceExt->FATStart+DeviceExt->Boot->FATCount*DeviceExt->Boot->FATSectors;
   DeviceExt->dataStart=DeviceExt->rootStart+DeviceExt->rootDirectorySectors;
   DeviceExt->FATEntriesPerSector=DeviceExt->Boot->BytesPerSector/32;
   DeviceExt->BytesPerCluster = DeviceExt->Boot->SectorsPerCluster *
                                DeviceExt->Boot->BytesPerSector;
   
   if (strncmp(DeviceExt->Boot->SysType,"FAT12",5)==0)
     {
	DeviceExt->FatType = FAT12;
     }
   else if (strncmp(((struct _BootSector32 *)(DeviceExt->Boot))->SysType,"FAT32",5)==0)
     {
      DeviceExt->FatType = FAT32;
      DeviceExt->rootDirectorySectors=DeviceExt->Boot->SectorsPerCluster;
      DeviceExt->rootStart=
             DeviceExt->FATStart+DeviceExt->Boot->FATCount
             * ((struct _BootSector32 *)( DeviceExt->Boot))->FATSectors32;
      DeviceExt->dataStart=DeviceExt->rootStart;
        }
   else
     {
	DeviceExt->FatType = FAT16;
     }

   // with FAT32 it's not a good idea to load always fat in memory
   // because on a 8GB partition with 2 KO clusters, the fat = 8 MO
   if(DeviceExt->FatType!=FAT32)
   {
    DeviceExt->FAT = ExAllocatePool(NonPagedPool, BLOCKSIZE*DeviceExt->Boot->FATSectors);
    VFATReadSectors(DeviceToMount, DeviceExt->FATStart, DeviceExt->Boot->FATSectors, (UCHAR *)DeviceExt->FAT);
   }
   return STATUS_SUCCESS;
}

void VFATLoadCluster(PDEVICE_EXTENSION DeviceExt, PVOID Buffer, ULONG Cluster)
/*
 * FUNCTION: Load a cluster from the physical device
 */
{
   ULONG Sector;

   DPRINT("VFATLoadCluster(DeviceExt %x, Buffer %x, Cluster %d)\n",
	  DeviceExt,Buffer,Cluster);

   Sector = ClusterToSector(DeviceExt, Cluster);

   VFATReadSectors(DeviceExt->StorageDevice,
 	           Sector,
                   DeviceExt->Boot->SectorsPerCluster,
	           Buffer);
}

void VFATWriteCluster(PDEVICE_EXTENSION DeviceExt, PVOID Buffer, ULONG Cluster)
/*
 * FUNCTION: Write a cluster to the physical device
 */
{
   ULONG Sector;
   DPRINT("VFATWriteCluster(DeviceExt %x, Buffer %x, Cluster %d)\n",
	  DeviceExt,Buffer,Cluster);
   
   Sector = ClusterToSector(DeviceExt, Cluster);
   
   VFATWriteSectors(DeviceExt->StorageDevice,
 	            Sector,
                    DeviceExt->Boot->SectorsPerCluster,
	            Buffer);
}

NTSTATUS FsdReadFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
		     PVOID Buffer, ULONG Length, ULONG ReadOffset,
		     PULONG LengthRead)
/*
 * FUNCTION: Reads data from a file
 */
{
   ULONG CurrentCluster;
   ULONG FileOffset;
   ULONG FirstCluster;
   PVfatFCB  Fcb;
   PVOID Temp;
   ULONG TempLength;
   
   /* PRECONDITION */
   assert(DeviceExt != NULL);
   assert(DeviceExt->BytesPerCluster != 0);
   assert(FileObject != NULL);
   assert(FileObject->FsContext != NULL);

   DPRINT("FsdReadFile(DeviceExt %x, FileObject %x, Buffer %x, "
	    "Length %d, ReadOffset %d)\n",DeviceExt,FileObject,Buffer,
	    Length,ReadOffset);
   
   Fcb = ((PVfatCCB)(FileObject->FsContext2))->pFcb;
   if (DeviceExt->FatType == FAT32)
	CurrentCluster = Fcb->entry.FirstCluster
                +Fcb->entry.FirstClusterHigh*65536;
   else
	CurrentCluster = Fcb->entry.FirstCluster;
   FirstCluster=CurrentCluster;
   DPRINT("DeviceExt->BytesPerCluster %x\n",DeviceExt->BytesPerCluster);
   
   if (ReadOffset >= Fcb->entry.FileSize
       && !(Fcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY))
     {
	return(STATUS_END_OF_FILE);
     }
   if ((ReadOffset + Length) > Fcb->entry.FileSize
       && !(Fcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY))
     {
	Length = Fcb->entry.FileSize - ReadOffset;
     }
   *LengthRead = 0;
   /* FIXME: optimize by remembering the last cluster read and using if possible */
   Temp = ExAllocatePool(NonPagedPool,DeviceExt->BytesPerCluster);
   if(!Temp) return STATUS_UNSUCCESSFUL;
   if (FirstCluster==1)
   {  //root of FAT16 or FAT12
     CurrentCluster=DeviceExt->rootStart+ReadOffset
              /(DeviceExt->BytesPerCluster)*DeviceExt->Boot->SectorsPerCluster;
   }
   else
     for (FileOffset=0; FileOffset < ReadOffset / DeviceExt->BytesPerCluster
           ; FileOffset++)
     {
       CurrentCluster = GetNextCluster(DeviceExt,CurrentCluster);
     }
   CHECKPOINT;
   if ((ReadOffset % DeviceExt->BytesPerCluster)!=0)
   {
    if (FirstCluster==1)
    {
      VFATReadSectors(DeviceExt->StorageDevice,CurrentCluster
           ,DeviceExt->Boot->SectorsPerCluster,Temp);
      CurrentCluster += DeviceExt->Boot->SectorsPerCluster;
    }
    else
    {
      VFATLoadCluster(DeviceExt,Temp,CurrentCluster);
      CurrentCluster = GetNextCluster(DeviceExt, CurrentCluster);
    }
	TempLength = min(Length,DeviceExt->BytesPerCluster -
			 (ReadOffset % DeviceExt->BytesPerCluster));
	
	memcpy(Buffer, Temp + ReadOffset % DeviceExt->BytesPerCluster,
	       TempLength);
	
	(*LengthRead) = (*LengthRead) + TempLength;
	Length = Length - TempLength;
	Buffer = Buffer + TempLength;	     
     }
   CHECKPOINT;
   while (Length >= DeviceExt->BytesPerCluster)
   {
    if (FirstCluster==1)
    {
      VFATReadSectors(DeviceExt->StorageDevice,CurrentCluster
           ,DeviceExt->Boot->SectorsPerCluster,Buffer);
      CurrentCluster += DeviceExt->Boot->SectorsPerCluster;
    }
    else
    {
      VFATLoadCluster(DeviceExt,Buffer,CurrentCluster);
      CurrentCluster = GetNextCluster(DeviceExt, CurrentCluster);
    }
      if (CurrentCluster == 0xffffffff)
	{
	   ExFreePool(Temp);
	   return(STATUS_SUCCESS);
	}
	
	(*LengthRead) = (*LengthRead) + DeviceExt->BytesPerCluster;
	Buffer = Buffer + DeviceExt->BytesPerCluster;
	Length = Length - DeviceExt->BytesPerCluster;
     }
   CHECKPOINT;
   if (Length > 0)
     {
	(*LengthRead) = (*LengthRead) + Length;
	if (FirstCluster==1)
	  {
	     VFATReadSectors(DeviceExt->StorageDevice,CurrentCluster
			     ,DeviceExt->Boot->SectorsPerCluster,Temp);
	     CurrentCluster += DeviceExt->Boot->SectorsPerCluster;
	  }
	else
	  {
	     VFATLoadCluster(DeviceExt,Temp,CurrentCluster);
	     CurrentCluster = GetNextCluster(DeviceExt, CurrentCluster);
	  }
	memcpy(Buffer, Temp, Length);
     }
   ExFreePool(Temp);
   return(STATUS_SUCCESS);
}

NTSTATUS FsdWriteFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
		      PVOID Buffer, ULONG Length, ULONG WriteOffset)
/*
 * FUNCTION: Writes data to file
 */
{
   ULONG CurrentCluster;
   ULONG FileOffset;
   ULONG FirstCluster;
   PVfatFCB  Fcb;
   PVfatCCB pCcb;
   PVOID Temp;
   ULONG TempLength,Length2=Length;

   /* Locate the first cluster of the file */
   assert(FileObject);
   pCcb=(PVfatCCB)(FileObject->FsContext2);
   assert(pCcb);
   Fcb = pCcb->pFcb;
   assert(Fcb);
   if (DeviceExt->FatType == FAT32)
	CurrentCluster = Fcb->entry.FirstCluster+Fcb->entry.FirstClusterHigh*65536;
   else
	CurrentCluster = Fcb->entry.FirstCluster;
   FirstCluster=CurrentCluster;
   /* Allocate a buffer to hold 1 cluster of data */

   Temp = ExAllocatePool(NonPagedPool,DeviceExt->BytesPerCluster);
   assert(Temp);

   /* Find the cluster according to the offset in the file */

   if (CurrentCluster==1)
   {  //root of FAT16 or FAT12
     CurrentCluster=DeviceExt->rootStart+WriteOffset
          /DeviceExt->BytesPerCluster*DeviceExt->Boot->SectorsPerCluster;
   }
   else
   if (CurrentCluster==0)
   {// file of size 0 : allocate first cluster
     CurrentCluster=GetNextWriteCluster(DeviceExt,0);
     if (DeviceExt->FatType == FAT32)
     {
       Fcb->entry.FirstClusterHigh=CurrentCluster>>16;
       Fcb->entry.FirstCluster=CurrentCluster;
     }
     else
       Fcb->entry.FirstCluster=CurrentCluster;
   }
   else
     for (FileOffset=0; FileOffset < WriteOffset / DeviceExt->BytesPerCluster; FileOffset++)
     {
       CurrentCluster = GetNextCluster(DeviceExt,CurrentCluster);
     }
   CHECKPOINT;

   /*
      If the offset in the cluster doesn't fall on the cluster boundary then
      we have to write only from the specified offset
   */

   if ((WriteOffset % DeviceExt->BytesPerCluster)!=0)
   {
   CHECKPOINT;
     TempLength = min(Length,DeviceExt->BytesPerCluster -
                        (WriteOffset % DeviceExt->BytesPerCluster));
     /* Read in the existing cluster data */
     if (FirstCluster==1)
       VFATReadSectors(DeviceExt->StorageDevice,CurrentCluster
           ,DeviceExt->Boot->SectorsPerCluster,Temp);
     else
       VFATLoadCluster(DeviceExt,Temp,CurrentCluster);

     /* Overwrite the last parts of the data as necessary */
     memcpy(Temp + (WriteOffset % DeviceExt->BytesPerCluster), Buffer,
	       TempLength);

     /* Write the cluster back */
     if (FirstCluster==1)
     {
       VFATWriteSectors(DeviceExt->StorageDevice,CurrentCluster
           ,DeviceExt->Boot->SectorsPerCluster,Temp);
       CurrentCluster += DeviceExt->Boot->SectorsPerCluster;
     }
     else
     {
       VFATWriteCluster(DeviceExt,Temp,CurrentCluster);
       CurrentCluster = GetNextCluster(DeviceExt, CurrentCluster);
     }
     Length2 -= TempLength;
     Buffer = Buffer + TempLength;
   }
   CHECKPOINT;

   /* Write the buffer in chunks of 1 cluster */

   while (Length2 >= DeviceExt->BytesPerCluster)
   {
   CHECKPOINT;
     if (CurrentCluster == 0)
     {
        ExFreePool(Temp);
        return(STATUS_UNSUCCESSFUL);
     }
     if (FirstCluster==1)
     {
       VFATWriteSectors(DeviceExt->StorageDevice,CurrentCluster
           ,DeviceExt->Boot->SectorsPerCluster,Buffer);
       CurrentCluster += DeviceExt->Boot->SectorsPerCluster;
     }
     else
     {
       VFATWriteCluster(DeviceExt,Buffer,CurrentCluster);
       CurrentCluster = GetNextCluster(DeviceExt, CurrentCluster);
     }
     Buffer = Buffer + DeviceExt->BytesPerCluster;
     Length2 -= DeviceExt->BytesPerCluster;
   }
   CHECKPOINT;

   /* Write the remainder */

   if (Length2 > 0)
   {
   CHECKPOINT;
     if (CurrentCluster == 0)
     {
        ExFreePool(Temp);
        return(STATUS_UNSUCCESSFUL);
     }
   CHECKPOINT;
     /* Read in the existing cluster data */
     if (FirstCluster==1)
       VFATReadSectors(DeviceExt->StorageDevice,CurrentCluster
           ,DeviceExt->Boot->SectorsPerCluster,Temp);
     else
       VFATLoadCluster(DeviceExt,Temp,CurrentCluster);
   CHECKPOINT;
     memcpy(Temp, Buffer, Length2);
   CHECKPOINT;
     if (FirstCluster==1)
     {
       VFATWriteSectors(DeviceExt->StorageDevice,CurrentCluster
           ,DeviceExt->Boot->SectorsPerCluster,Temp);
     }
     else
       VFATWriteCluster(DeviceExt,Temp,CurrentCluster);
   }
   CHECKPOINT;
//FIXME : set  last write time and date
   if(Fcb->entry.FileSize<WriteOffset+Length
       && !(Fcb->entry.Attrib &FILE_ATTRIBUTE_DIRECTORY))
   {
     Fcb->entry.FileSize=WriteOffset+Length;
     // update entry in directory
     updEntry(DeviceExt,FileObject);
   }
   ExFreePool(Temp);
   return(STATUS_SUCCESS);
}

NTSTATUS FsdClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Close a file
 */
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PFILE_OBJECT FileObject = Stack->FileObject;
   PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
   NTSTATUS Status;

   Status = FsdCloseFile(DeviceExtension,FileObject);

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(Status);
}

NTSTATUS FsdCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Create or open a file
 */
{
 PIO_STACK_LOCATION Stack;
 PFILE_OBJECT FileObject;
 NTSTATUS Status=STATUS_SUCCESS;
 PDEVICE_EXTENSION DeviceExt;
 ULONG RequestedDisposition,RequestedOptions;
 PVfatCCB pCcb;
 PVfatFCB pFcb;

   assert(DeviceObject);
   assert(Irp);
   if(DeviceObject->Size==sizeof(DEVICE_OBJECT))
   {// DevieObject represent FileSystem instead of  logical volume
     DbgPrint("FsdCreate called with file system\n");
     Irp->IoStatus.Status=Status;
     Irp->IoStatus.Information=FILE_OPENED;
     IoCompleteRequest(Irp,IO_NO_INCREMENT);
     return(Status);
   }
   Stack = IoGetCurrentIrpStackLocation(Irp);
   assert(Stack);
   RequestedDisposition = ((Stack->Parameters.Create.Options>>24)&0xff);
   RequestedOptions=Stack->Parameters.Create.Options&FILE_VALID_OPTION_FLAGS;
   FileObject = Stack->FileObject;
   DeviceExt = DeviceObject->DeviceExtension;
   assert(DeviceExt);
   ExAcquireResourceExclusiveLite(&(DeviceExt->Resource),TRUE);
   Status = FsdOpenFile(DeviceExt,FileObject,FileObject->FileName.Buffer);
   Irp->IoStatus.Information = 0;
   if(!NT_SUCCESS(Status))
   {
      if(RequestedDisposition==FILE_CREATE
         ||RequestedDisposition==FILE_OPEN_IF
         ||RequestedDisposition==FILE_OVERWRITE_IF)
      {
         Status=addEntry(DeviceExt,FileObject,RequestedOptions
             ,(Stack->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_VALID_FLAGS));
         if(NT_SUCCESS(Status))
           Irp->IoStatus.Information = FILE_CREATED;
         // FIXME set size if AllocationSize requested
         // FIXME set extended attributes ?
         // FIXME set share access
         // IoSetShareAccess(DesiredAccess,ShareAccess,FileObject
         //   ,((PVfatCCB)(FileObject->FsContext2))->pFcb->FCBShareAccess);
      }
   }
   else
   {
     if(RequestedDisposition==FILE_CREATE)
     {
       Irp->IoStatus.Information = FILE_EXISTS;
       Status=STATUS_OBJECT_NAME_COLLISION;
     }
     pCcb=FileObject->FsContext2;
     pFcb=pCcb->pFcb;
     if( (RequestedOptions&FILE_NON_DIRECTORY_FILE)
         && (pFcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY))
     {
       Status=STATUS_FILE_IS_A_DIRECTORY;
     }
     if( (RequestedOptions&FILE_DIRECTORY_FILE)
         && !(pFcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY))
     {
       Status=STATUS_NOT_A_DIRECTORY;
     }
     // FIXME : test share access
     // FIXME : test write access if requested
     if(!NT_SUCCESS(Status))
       FsdCloseFile(DeviceExt,FileObject);
     else Irp->IoStatus.Information = FILE_OPENED;
     // FIXME : make supersed or overwrite if requested
   }
   
   Irp->IoStatus.Status = Status;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   ExReleaseResourceForThreadLite(&(DeviceExt->Resource),ExGetCurrentResourceThread());
   return Status;
}


NTSTATUS FsdWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Write to a file
 */
{
   ULONG Length;
   PVOID Buffer;
   ULONG Offset;
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PFILE_OBJECT FileObject = Stack->FileObject;
   PDEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
   NTSTATUS Status;
   
   DPRINT("FsdWrite(DeviceObject %x Irp %x)\n",DeviceObject,Irp);

   Length = Stack->Parameters.Write.Length;
   Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
   Offset = GET_LARGE_INTEGER_LOW_PART(Stack->Parameters.Write.ByteOffset);

   Status = FsdWriteFile(DeviceExt,FileObject,Buffer,Length,Offset);

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = Length;
   IoCompleteRequest(Irp,IO_NO_INCREMENT);

   return(Status);
}

NTSTATUS FsdRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Read from a file
 */
{
   ULONG Length;
   PVOID Buffer;
   ULONG Offset;
   PIO_STACK_LOCATION Stack;
   PFILE_OBJECT FileObject;
   PDEVICE_EXTENSION DeviceExt;
   NTSTATUS Status;
   ULONG LengthRead;
   
   DPRINT("FsdRead(DeviceObject %x, Irp %x)\n",DeviceObject,Irp);

   /* Precondition / Initialization */
   assert(Irp != NULL);
   Stack = IoGetCurrentIrpStackLocation(Irp);
   assert(Stack != NULL);
   FileObject = Stack->FileObject;
   assert(FileObject != NULL);
   DeviceExt = DeviceObject->DeviceExtension;
   assert(DeviceExt != NULL);

   Length = Stack->Parameters.Read.Length;
   Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
   Offset = GET_LARGE_INTEGER_LOW_PART(Stack->Parameters.Read.ByteOffset);
   
   Status = FsdReadFile(DeviceExt,FileObject,Buffer,Length,Offset,
			&LengthRead);
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = LengthRead;
   IoCompleteRequest(Irp,IO_NO_INCREMENT);

   return(Status);
}


NTSTATUS FsdMount(PDEVICE_OBJECT DeviceToMount)
/*
 * FUNCTION: Mount the filesystem
 */
{
   PDEVICE_OBJECT DeviceObject;
   PDEVICE_EXTENSION DeviceExt;
      
   IoCreateDevice(VFATDriverObject,
		  sizeof(DEVICE_EXTENSION),
		  NULL,
		  FILE_DEVICE_FILE_SYSTEM,
		  0,
		  FALSE,
		  &DeviceObject);
   DeviceObject->Flags = DeviceObject->Flags | DO_DIRECT_IO;
   DeviceExt = (PVOID)DeviceObject->DeviceExtension;
   // use same vpb as device disk
   DeviceObject->Vpb=DeviceToMount->Vpb;
   FsdMountDevice(DeviceExt,DeviceToMount);
   DeviceObject->Vpb->Flags |= VPB_MOUNTED;
   DeviceExt->StorageDevice = IoAttachDeviceToDeviceStack(DeviceObject,
							  DeviceToMount);
   return(STATUS_SUCCESS);
}

NTSTATUS FsdFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: File system control
 */
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
//   PVPB	vpb = Stack->Parameters.Mount.Vpb;
   PDEVICE_OBJECT DeviceToMount = Stack->Parameters.Mount.DeviceObject;
   NTSTATUS Status;

   DPRINT("VFAT FSC\n");

   /* FIXME: should make sure that this is actually a mount request!  */

   if (FsdHasFileSystem(DeviceToMount))
     {
	Status = FsdMount(DeviceToMount);
     }
   else
     {
        DPRINT("VFAT: Unrecognized Volume\n");
	Status = STATUS_UNRECOGNIZED_VOLUME;
     }
   DPRINT("VFAT File system successfully mounted\n");

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(Status);
}

NTSTATUS FsdGetStandardInformation(PVfatFCB FCB, PDEVICE_OBJECT DeviceObject,
                                   PFILE_STANDARD_INFORMATION StandardInfo)
/*
 * FUNCTION: Retrieve the standard file information
 */
{
  PDEVICE_EXTENSION DeviceExtension;
  unsigned long AllocSize;

  DeviceExtension = DeviceObject->DeviceExtension;
  /* PRECONDITION */
  assert(DeviceExtension != NULL);
  assert(DeviceExtension->BytesPerCluster != 0);
  assert(StandardInfo != NULL);
  assert(FCB != NULL);

  RtlZeroMemory(StandardInfo, sizeof(FILE_STANDARD_INFORMATION));

  /* Make allocsize a rounded up multiple of BytesPerCluster */
  AllocSize = ((FCB->entry.FileSize +  DeviceExtension->BytesPerCluster - 1) /
              DeviceExtension->BytesPerCluster) *
              DeviceExtension->BytesPerCluster;

  StandardInfo->AllocationSize = RtlConvertUlongToLargeInteger(AllocSize);
  StandardInfo->EndOfFile      = RtlConvertUlongToLargeInteger(FCB->entry.FileSize);
  StandardInfo->NumberOfLinks  = 0;
  StandardInfo->DeletePending  = FALSE;
  if((FCB->entry.Attrib & 0x10)>0) {
    StandardInfo->Directory    = TRUE;
  } else {
    StandardInfo->Directory    = FALSE;
  }

  return STATUS_SUCCESS;
}

NTSTATUS FsdQueryInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Retrieve the specified file information
 */
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   FILE_INFORMATION_CLASS FileInformationClass =
     Stack->Parameters.QueryFile.FileInformationClass;
   PFILE_OBJECT FileObject = NULL;
   PVfatFCB FCB = NULL;
//   PVfatCCB CCB = NULL;

   NTSTATUS RC = STATUS_SUCCESS;
   void *SystemBuffer;

   /* PRECONDITION */
   assert(DeviceObject != NULL);
   assert(Irp != NULL);

   /* INITIALIZATION */
   Stack = IoGetCurrentIrpStackLocation(Irp);
   FileInformationClass = Stack->Parameters.QueryFile.FileInformationClass;
   FileObject = Stack->FileObject;
//   CCB = (PVfatCCB)(FileObject->FsContext2);
//   FCB = CCB->Buffer; // Should be CCB->FCB???
   FCB = ((PVfatCCB)(FileObject->FsContext2))->pFcb;

  // FIXME : determine Buffer for result :
  if (Irp->MdlAddress) 
    SystemBuffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
  else
    SystemBuffer = Irp->UserBuffer;
//   SystemBuffer = Irp->AssociatedIrp.SystemBuffer;

   switch(FileInformationClass) {
      case FileStandardInformation:
         RC = FsdGetStandardInformation(FCB, DeviceObject, SystemBuffer);
      break;
      default:
       RC=STATUS_NOT_IMPLEMENTED;
   }

   return RC;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT _DriverObject,
		     PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Called by the system to initalize the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
{
   PDEVICE_OBJECT DeviceObject;
   NTSTATUS ret;
   UNICODE_STRING ustr;
   ANSI_STRING astr;
   
   DbgPrint("VFAT 0.0.6\n");
   pFirstFcb=NULL;
   VFATDriverObject = _DriverObject;
   
   RtlInitAnsiString(&astr,"\\Device\\VFAT");
   RtlAnsiStringToUnicodeString(&ustr,&astr,TRUE);
   ret = IoCreateDevice(VFATDriverObject,0,&ustr,
                        FILE_DEVICE_FILE_SYSTEM,0,FALSE,&DeviceObject);
   if (ret!=STATUS_SUCCESS)
     {
	return(ret);
     }

   DeviceObject->Flags = DO_DIRECT_IO;
   VFATDriverObject->MajorFunction[IRP_MJ_CLOSE] = FsdClose;
   VFATDriverObject->MajorFunction[IRP_MJ_CREATE] = FsdCreate;
   VFATDriverObject->MajorFunction[IRP_MJ_READ] = FsdRead;
   VFATDriverObject->MajorFunction[IRP_MJ_WRITE] = FsdWrite;
   VFATDriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
                      FsdFileSystemControl;
   VFATDriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
                      FsdQueryInformation;
   VFATDriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] =
                      FsdDirectoryControl;

   VFATDriverObject->DriverUnload = NULL;
   
   IoRegisterFileSystem(DeviceObject);

   return(STATUS_SUCCESS);
}

