/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/dirwr.c
 * PURPOSE:          VFAT Filesystem : write in directory

*/

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/string.h>
#include <internal/ctype.h>
#include <wstring.h>
#include <ddk/cctypes.h>

//#define NDEBUG
#include <internal/debug.h>

#include "vfat.h"

NTSTATUS updEntry(PDEVICE_EXTENSION DeviceExt,PFILE_OBJECT pFileObject)
/*
  update an existing FAT entry
*/
{
 WCHAR DirName[MAX_PATH],*FileName,*PathFileName;
 VfatFCB FileFcb;
 ULONG Sector=0,Entry=0;
 PUCHAR Buffer;
 FATDirEntry * pEntries;
 NTSTATUS status;
 FILE_OBJECT FileObject;
 PVfatFCB pDirFcb,pFcb;
 short i,posCar,NameLen;
   PathFileName=pFileObject->FileName.Buffer;
   pFcb=((PVfatCCB)pFileObject->FsContext2)->pFcb;
   //find last \ in PathFileName
   posCar=-1;
   for(i=0;PathFileName[i];i++)
     if(PathFileName[i]=='\\')posCar=i;
   if(posCar==-1)
     return STATUS_UNSUCCESSFUL;
   FileName=&PathFileName[posCar+1];
   for(NameLen=0;FileName[NameLen];NameLen++);
   // extract directory name from pathname
   memcpy(DirName,FileName,posCar*sizeof(WCHAR));
   DirName[posCar]=0;
   if(FileName[0]==0 && DirName[0]==0)
     return STATUS_SUCCESS;//root : nothing to do ?
   status=FsdOpenFile(DeviceExt,&FileObject,DirName);
   pDirFcb=((PVfatCCB)FileObject.FsContext2)->pFcb;
   FileFcb.ObjectName=&FileFcb.PathName[0];
   status=FindFile(DeviceExt,&FileFcb,pDirFcb,FileName,&Sector,&Entry);
   Buffer=ExAllocatePool(NonPagedPool,BLOCKSIZE);
DPRINT("update entry: sector %d, entry %d\n",Sector,Entry);
   VFATReadSectors(DeviceExt->StorageDevice,Sector,1,Buffer);
   pEntries=(FATDirEntry *)Buffer;
   memcpy(&pEntries[Entry],&pFcb->entry,sizeof(FATDirEntry));
   VFATWriteSectors(DeviceExt->StorageDevice,Sector,1,Buffer);
   ExFreePool(Buffer);
   FsdCloseFile(DeviceExt,&FileObject);
   return STATUS_SUCCESS;
}

NTSTATUS addEntry(PDEVICE_EXTENSION DeviceExt
                  ,PFILE_OBJECT pFileObject,ULONG RequestedOptions)
/*
  create a new FAT entry
*/
{
 WCHAR DirName[MAX_PATH],*FileName,*PathFileName;
 VfatFCB DirFcb,FileFcb;
 FATDirEntry FatEntry;
 NTSTATUS status;
 FILE_OBJECT FileObject;
 FATDirEntry *pEntry;
 slot *pSlots;
 ULONG LengthRead,Offset;
 short nbSlots=0,nbFree=0,i,j,posCar,NameLen;
 PUCHAR Buffer,Buffer2;
 BOOLEAN needTilde;
 PVfatFCB newFCB;
 PVfatCCB newCCB;
 ULONG CurrentCluster;
   PathFileName=pFileObject->FileName.Buffer;
   DPRINT("addEntry: Pathname=%w\n",PathFileName);
   //find last \ in PathFileName
   posCar=-1;
   for(i=0;PathFileName[i];i++)
     if(PathFileName[i]=='\\')posCar=i;
   if(posCar==-1)
     return STATUS_UNSUCCESSFUL;
   FileName=&PathFileName[posCar+1];
   for(NameLen=0;FileName[NameLen];NameLen++);
   // extract directory name from pathname
   memcpy(DirName,PathFileName,posCar*sizeof(WCHAR));
   DirName[posCar]=0;
   DPRINT("open directory %w\n",DirName);
   status=FsdOpenFile(DeviceExt,&FileObject,DirName);
   DPRINT("after open, status=%x\n",status);
   nbSlots=(NameLen+12)/13+1;
   DPRINT("NameLen= %d, nbSlots =%d\n",NameLen,nbSlots);
   Buffer=ExAllocatePool(NonPagedPool,(nbSlots+1)*sizeof(FATDirEntry));
   memset(Buffer,0,(nbSlots+1)*sizeof(FATDirEntry));
   pEntry=(FATDirEntry *)(Buffer+(nbSlots-1)*sizeof(FATDirEntry));
   pSlots=(slot *)Buffer;
   // create 8.3 name
   needTilde=FALSE;
   // find last point in name
   posCar=0;
   for(i=0;FileName[i];i++)
     if(FileName[i]=='.')posCar=i;
   if(!posCar) posCar=i;
   if(posCar>8) needTilde=TRUE;
   //copy 8 characters max
   memset(pEntry,' ',11);
   for(i=0,j=0;j<8 && i<posCar;i++)
   {
     //FIXME : is there other characters to ignore ?
     if(   FileName[i]!='.'
        && FileName[i]!=' '
        && FileName[i]!='+'
        && FileName[i]!=','
        && FileName[i]!=';'
        && FileName[i]!='='
        && FileName[i]!='['
        && FileName[i]!=']'
        )
       pEntry->Filename[j++]=toupper((char) FileName[i]);
     else
       needTilde=TRUE;
   }
   //copy extension
   if(FileName[posCar])
     for(j=0,i=posCar+1;i<posCar+4;i++)
     {
       pEntry->Ext[j++]=toupper((char)( FileName[i] &0x7F));
     }
   //find good value for tilde
   if(needTilde)
   {
      DPRINT("searching a good value for tilde\n");
      for(i=0;i<6;i++)
        DirName[i]=pEntry->Filename[i];
      for(i=0;i<3;i++)
        DirName[i+8]=pEntry->Ext[i];
      //try first with xxxxxx~y.zzz
      DirName[6]='~';
      pEntry->Filename[6]='~';
      DirName[8]='.';
      DirName[12]=0;
      for(i=1;i<9;i++)
      {
         DirName[7]='0'+i;
         pEntry->Filename[7]='0'+i;
         status=FindFile(DeviceExt,&FileFcb
           ,&DirFcb,DirName,NULL,NULL);
         if(status!=STATUS_SUCCESS)break;
      }
      //try second with xxxxx~yy.zzz
      if(i==10)
      {
        DirName[5]='~';
        for( ;i<99;i++)
        {
         DirName[7]='0'+i;
         pEntry->Filename[7]='0'+i;
         status=FindFile(DeviceExt,&FileFcb
           ,&DirFcb,DirName,NULL,NULL);
         if(status!=STATUS_SUCCESS)break;
        }
      }
      if(i==100)//FIXME : what to do after 99 tilde ?
      {
         FsdCloseFile(DeviceExt,&FileObject);
         ExFreePool(Buffer);
         return STATUS_UNSUCCESSFUL;
      }
   }
   DPRINT("dos name=%11.11s\n",pEntry->Filename);
   //FIXME : set attributes, dates, times
   if(RequestedOptions&FILE_DIRECTORY_FILE)
     pEntry->Attrib |= FILE_ATTRIBUTE_DIRECTORY;
   pEntry->CreationDate=0x21;
   pEntry->UpdateDate=0x21;
   //calculate checksum for 8.3 name
   //construct slots and entry
   for(i=nbSlots-2;i>=0;i--)
   {
      DPRINT("construct slot %d\n",i);
      pSlots[i].attr=0xf;
      if (i)
        pSlots[i].id=nbSlots-i-1;
      else
        pSlots[i].id=nbSlots-i-1+0x40;
//      pSlots[i].alias_checksum=;
//      pSlots[i].start=;
      memcpy(pSlots[i].name0_4  ,FileName+(nbSlots-i-2)*13
         ,5*sizeof(WCHAR));
      memcpy(pSlots[i].name5_10 ,FileName+(nbSlots-i-2)*13+5
         ,6*sizeof(WCHAR));
      memcpy(pSlots[i].name11_12,FileName+(nbSlots-i-2)*13+11
         ,2*sizeof(WCHAR));
   }
   //try to find nbSlots contiguous entries frees in directory
   for(i=0,status=STATUS_SUCCESS;status==STATUS_SUCCESS;i++)
   {
      status=FsdReadFile(DeviceExt,&FileObject,&FatEntry
           ,sizeof(FATDirEntry),i*sizeof(FATDirEntry),&LengthRead);
      if(IsLastEntry(&FatEntry,0))
        break;
      if(IsDeletedEntry(&FatEntry,0)) nbFree++;
      else nbFree=0;
      if (nbFree==nbSlots) break;
   }
   DPRINT("NbFree %d, entry number %d\n",nbFree,i);
   CurrentCluster=GetNextWriteCluster(DeviceExt,0);
   // zero the cluster
   Buffer2=ExAllocatePool(NonPagedPool,DeviceExt->BytesPerCluster);
   memset(Buffer2,0,DeviceExt->BytesPerCluster);
   VFATWriteCluster(DeviceExt,Buffer2,CurrentCluster);
   ExFreePool(Buffer2);
   if (DeviceExt->FatType == FAT32)
   {
     pEntry->FirstClusterHigh=CurrentCluster>>16;
     pEntry->FirstCluster=CurrentCluster;
   }
   else
     pEntry->FirstCluster=CurrentCluster;
   if(nbFree==nbSlots)
   {//use old slots
     Offset=(i-nbSlots+1)*sizeof(FATDirEntry);
     status=FsdWriteFile(DeviceExt,&FileObject,Buffer
          ,sizeof(FATDirEntry)*nbSlots,Offset);
   }
   else
   {//write at end of directory
     Offset=(i-nbFree)*sizeof(FATDirEntry);
     status=FsdWriteFile(DeviceExt,&FileObject,Buffer
          ,sizeof(FATDirEntry)*(nbSlots+1),Offset);
   }
   DPRINT("write entry offset %d status=%x\n",Offset,status);
   newCCB = ExAllocatePool(NonPagedPool,sizeof(VfatCCB));
   newFCB = ExAllocatePool(NonPagedPool,sizeof(VfatFCB));
   memset(newCCB,0,sizeof(VfatCCB));
   memset(newFCB,0,sizeof(VfatFCB));
   newCCB->pFcb=newFCB;
   newCCB->PtrFileObject=pFileObject;
   newFCB->RefCount++;
   //FIXME : initialize all fields in FCB and CCB
   newFCB->nextFcb=pFirstFcb;
   memcpy(&newFCB->entry,pEntry,sizeof(FATDirEntry));
DPRINT("new : entry=%11.11s\n",newFCB->entry.Filename);
DPRINT("new : entry=%11.11s\n",pEntry->Filename);
   pFirstFcb=newFCB;
   vfat_wcsncpy(newFCB->PathName,PathFileName,MAX_PATH);
   newFCB->ObjectName=newFCB->PathName+(PathFileName-FileName);
   newFCB->pDevExt=DeviceExt;
   pFileObject->FsContext =(PVOID) &newFCB->NTRequiredFCB;
   pFileObject->FsContext2 = newCCB;
   if(RequestedOptions&FILE_DIRECTORY_FILE)
   {
     // create . and ..
     memcpy(pEntry->Filename,".          ",11);
     status=FsdWriteFile(DeviceExt,pFileObject,pEntry
          ,sizeof(FATDirEntry),0L);
     pEntry->FirstCluster
            =((VfatCCB *)(FileObject.FsContext2))->pFcb->entry.FirstCluster;
     pEntry->FirstClusterHigh
            =((VfatCCB *)(FileObject.FsContext2))->pFcb->entry.FirstClusterHigh;
     memcpy(pEntry->Filename,"..         ",11);
     if(pEntry->FirstCluster==1 && DeviceExt->FatType!=FAT32)
       pEntry->FirstCluster=0;
     status=FsdWriteFile(DeviceExt,pFileObject,pEntry
          ,sizeof(FATDirEntry),sizeof(FATDirEntry));
   }
   FsdCloseFile(DeviceExt,&FileObject);
   ExFreePool(Buffer);
DPRINT("addentry ok\n");
   return STATUS_SUCCESS;
}

