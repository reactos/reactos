/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/dir.c
 * PURPOSE:          VFAT Filesystem : directory control
 * UPDATE HISTORY:
     19-12-1998 : created

*/
#include <ddk/ntddk.h>
#include <internal/string.h>
#include <wstring.h>
#include <ddk/cctypes.h>
#include <ddk/zwtypes.h>

#define NDEBUG
#include <internal/debug.h>

#include "vfat.h"

//days from 1st January
static const int MonthsDF1[2][12] =
{
    { 0,31, 59, 90,120,151,181,212,243,273,304,334 },
    { 0,31, 60, 91,121,152,182,213,244,274,305,335 }
};
static __inline int IsLeapYear(int Year)
{
  return Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0) ? 1 : 0;
}
// function like DosDateTimeToFileTime
BOOL fsdDosDateTimeToFileTime(WORD wDosDate,WORD wDosTime, TIME *FileTime)
{
 WORD Day,Month,Year,Second,Minute,Hour;
 long long int *pTime=(long long int *)FileTime;
 long long int mult;
  Day=wDosDate&0x001f;
  Month= (wDosDate&0x00e0)>>5;//1=January
  Year= ((wDosDate&0xff00)>>8)+1980;
  Second=(wDosTime&0x001f)<<1;
  Minute=(wDosTime&0x07e0)>>5;
  Hour=  (wDosTime&0xf100)>>11;
  mult=10000000;
  *pTime=Second*mult;
  mult *=60;
  *pTime +=Minute*mult;
  mult *=60;
  *pTime +=Hour*mult;
  mult *=24;
  *pTime +=(Day-1)*mult;
  if((Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0) ? 1 : 0))
    *pTime += MonthsDF1[1][Month-1];
  else
    *pTime += MonthsDF1[0][Month-1];
  *pTime +=(Year-1601)*mult*365
           +(Year-1601)/4
           -(Year-1601)/100
           +(Year-1601)/400;
  return TRUE;
}
#define DosDateTimeToFileTime fsdDosDateTimeToFileTime




unsigned long vfat_wstrlen(PWSTR s)
{
        WCHAR c=' ';
        unsigned int len=0;

        while(c!=0) {
                c=*s;
                s++;
                len++;
        };
        s-=len;

        return len-1;
}
#define DWORD_ROUND_UP(x) ( (((ULONG)(x))%32) ? ((((ULONG)x)&(~0x1f))+0x20) : ((ULONG)x) )

NTSTATUS FsdGetFileNameInformation(PVfatFCB pFcb,
         PFILE_NAMES_INFORMATION pInfo,ULONG BufferLength)
{
 ULONG Length;
  Length=vfat_wstrlen(pFcb->ObjectName);
  if( (sizeof(FILE_DIRECTORY_INFORMATION)+Length) >BufferLength)
     return STATUS_BUFFER_OVERFLOW;
  pInfo->FileNameLength=Length;
  pInfo->NextEntryOffset=DWORD_ROUND_UP(sizeof(FILE_DIRECTORY_INFORMATION)+Length);
  memcpy(pInfo->FileName,pFcb->ObjectName
     ,sizeof(WCHAR)*(pInfo->FileNameLength));
  return STATUS_SUCCESS;
}

NTSTATUS FsdGetFileDirectoryInformation(PVfatFCB pFcb,
          PDEVICE_EXTENSION DeviceExt,
          PFILE_DIRECTORY_INFORMATION pInfo,ULONG BufferLength)
{
 unsigned long long AllocSize;
 ULONG Length;
  Length=vfat_wstrlen(pFcb->ObjectName);
  if( (sizeof(FILE_DIRECTORY_INFORMATION)+Length) >BufferLength)
     return STATUS_BUFFER_OVERFLOW;
  pInfo->FileNameLength=Length;
  pInfo->NextEntryOffset=DWORD_ROUND_UP(sizeof(FILE_DIRECTORY_INFORMATION)+Length);
  memcpy(pInfo->FileName,pFcb->ObjectName
     ,sizeof(WCHAR)*(pInfo->FileNameLength));
//      pInfo->FileIndex=;
  DosDateTimeToFileTime(pFcb->entry.CreationDate,pFcb->entry.CreationTime
      ,&pInfo->CreationTime);
  DosDateTimeToFileTime(pFcb->entry.AccessDate,0
      ,&pInfo->LastAccessTime);
  DosDateTimeToFileTime(pFcb->entry.UpdateDate,pFcb->entry.UpdateTime
      ,&pInfo->LastWriteTime);
  DosDateTimeToFileTime(pFcb->entry.UpdateDate,pFcb->entry.UpdateTime
      ,&pInfo->ChangeTime);
  pInfo->EndOfFile=RtlConvertUlongToLargeInteger(pFcb->entry.FileSize);
  /* Make allocsize a rounded up multiple of BytesPerCluster */
  AllocSize = ((pFcb->entry.FileSize +  DeviceExt->BytesPerCluster - 1) /
          DeviceExt->BytesPerCluster) *
          DeviceExt->BytesPerCluster;
  LARGE_INTEGER_QUAD_PART(pInfo->AllocationSize) = AllocSize;
  pInfo->FileAttributes=pFcb->entry.Attrib;

  return STATUS_SUCCESS;
}

NTSTATUS FsdGetFileFullDirectoryInformation(PVfatFCB pFcb,
          PDEVICE_EXTENSION DeviceExt,
          PFILE_FULL_DIRECTORY_INFORMATION pInfo,ULONG BufferLength)
{
 unsigned long long AllocSize;
 ULONG Length;
  Length=vfat_wstrlen(pFcb->ObjectName);
  if( (sizeof(FILE_FULL_DIRECTORY_INFORMATION)+Length) >BufferLength)
     return STATUS_BUFFER_OVERFLOW;
  pInfo->FileNameLength=Length;
  pInfo->NextEntryOffset=DWORD_ROUND_UP(sizeof(FILE_FULL_DIRECTORY_INFORMATION)+Length);
  memcpy(pInfo->FileName,pFcb->ObjectName
     ,sizeof(WCHAR)*(pInfo->FileNameLength));
//      pInfo->FileIndex=;
  DosDateTimeToFileTime(pFcb->entry.CreationDate,pFcb->entry.CreationTime
      ,&pInfo->CreationTime);
  DosDateTimeToFileTime(pFcb->entry.AccessDate,0
      ,&pInfo->LastAccessTime);
  DosDateTimeToFileTime(pFcb->entry.UpdateDate,pFcb->entry.UpdateTime
      ,&pInfo->LastWriteTime);
  DosDateTimeToFileTime(pFcb->entry.UpdateDate,pFcb->entry.UpdateTime
      ,&pInfo->ChangeTime);
  pInfo->EndOfFile=RtlConvertUlongToLargeInteger(pFcb->entry.FileSize);
  /* Make allocsize a rounded up multiple of BytesPerCluster */
  AllocSize = ((pFcb->entry.FileSize +  DeviceExt->BytesPerCluster - 1) /
          DeviceExt->BytesPerCluster) *
          DeviceExt->BytesPerCluster;
  LARGE_INTEGER_QUAD_PART(pInfo->AllocationSize) = AllocSize;
  pInfo->FileAttributes=pFcb->entry.Attrib;
//      pInfo->EaSize=;
  return STATUS_SUCCESS;
}

NTSTATUS FsdGetFileBothInformation(PVfatFCB pFcb,
          PDEVICE_EXTENSION DeviceExt,
          PFILE_BOTH_DIRECTORY_INFORMATION pInfo,ULONG BufferLength)
{
 short i;
 unsigned long long AllocSize;
 ULONG Length;
  Length=vfat_wstrlen(pFcb->ObjectName);
  if( (sizeof(FILE_BOTH_DIRECTORY_INFORMATION)+Length) >BufferLength)
     return STATUS_BUFFER_OVERFLOW;
  pInfo->FileNameLength=Length;
  pInfo->NextEntryOffset=DWORD_ROUND_UP(sizeof(FILE_BOTH_DIRECTORY_INFORMATION)+Length);
  memcpy(pInfo->FileName,pFcb->ObjectName
     ,sizeof(WCHAR)*(pInfo->FileNameLength));
//      pInfo->FileIndex=;
  DosDateTimeToFileTime(pFcb->entry.CreationDate,pFcb->entry.CreationTime
      ,&pInfo->CreationTime);
  DosDateTimeToFileTime(pFcb->entry.AccessDate,0
      ,&pInfo->LastAccessTime);
  DosDateTimeToFileTime(pFcb->entry.UpdateDate,pFcb->entry.UpdateTime
      ,&pInfo->LastWriteTime);
  DosDateTimeToFileTime(pFcb->entry.UpdateDate,pFcb->entry.UpdateTime
      ,&pInfo->ChangeTime);
  pInfo->EndOfFile=RtlConvertUlongToLargeInteger(pFcb->entry.FileSize);
  /* Make allocsize a rounded up multiple of BytesPerCluster */
  AllocSize = ((pFcb->entry.FileSize +  DeviceExt->BytesPerCluster - 1) /
          DeviceExt->BytesPerCluster) *
          DeviceExt->BytesPerCluster;
  LARGE_INTEGER_QUAD_PART(pInfo->AllocationSize) = AllocSize;
  pInfo->FileAttributes=pFcb->entry.Attrib;
//      pInfo->EaSize=;
  for (i=0;i<8 && (pFcb->entry.Filename[i]!=' ') ;i++)
    pInfo->ShortName[i]=pFcb->entry.Filename[i];
  pInfo->ShortNameLength=i;
  pInfo->ShortName[i]='.';
  for (i=0 ;i<3 && (pFcb->entry.Ext[i]!=' ') ;i++)
    pInfo->ShortName[i+1+pInfo->ShortNameLength]=pFcb->entry.Ext[i];
  if(i) pInfo->ShortNameLength += (i+1);
  return STATUS_SUCCESS;
}

NTSTATUS DoQuery(PDEVICE_OBJECT DeviceObject, PIRP Irp,PIO_STACK_LOCATION Stack)
{
 NTSTATUS RC=STATUS_SUCCESS;
 long BufferLength = 0;
 PUNICODE_STRING pSearchPattern = NULL;
 FILE_INFORMATION_CLASS FileInformationClass;
 unsigned long FileIndex = 0;
 unsigned char *Buffer = NULL;
 PFILE_NAMES_INFORMATION Buffer0 = NULL;
 PFILE_OBJECT pFileObject = NULL;
 PVfatFCB pFcb;
 VfatFCB tmpFcb;
 PVfatCCB pCcb;
 PDEVICE_EXTENSION DeviceExt;
 WCHAR star[5],*pCharPattern;
 unsigned long OldEntry;
  DeviceExt = DeviceObject->DeviceExtension;
  // Obtain the callers parameters
  BufferLength = Stack->Parameters.QueryDirectory.Length;
  pSearchPattern = Stack->Parameters.QueryDirectory.FileName;
  FileInformationClass = Stack->Parameters.QueryDirectory.FileInformationClass;
  FileIndex = Stack->Parameters.QueryDirectory.FileIndex;
  pFileObject = Stack->FileObject;
  pCcb =(PVfatCCB)pFileObject->FsContext2;
  pFcb = pCcb->pFcb;
  if(Stack->Flags & SL_RESTART_SCAN)
  {//FIXME : what is really use of RestartScan ?
    pCcb->StartEntry=0;
  }
  // determine Buffer for result :
  if (Irp->MdlAddress) 
    Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
  else
    Buffer = Irp->UserBuffer;
  DPRINT("Buffer=%x tofind=%w\n",Buffer,pSearchPattern->Buffer);
  if (pSearchPattern==NULL)
  {
    star[0]='*';
    star[1]=0;
    pCharPattern=star;
  }
  else pCharPattern=pSearchPattern->Buffer;
  tmpFcb.ObjectName=tmpFcb.PathName;
  while(RC==STATUS_SUCCESS && BufferLength >0)
  {
    OldEntry=pCcb->StartEntry;
    RC=FindFile(DeviceExt,&tmpFcb,pFcb,pCharPattern,&pCcb->StartEntry);
DPRINT("Found %w,RC=%x,entry %x\n",tmpFcb.ObjectName,RC
 ,pCcb->StartEntry);
    pCcb->StartEntry++;
    if (NT_SUCCESS(RC))
    {
      switch(FileInformationClass)
      {
       case FileNameInformation:
        RC=FsdGetFileNameInformation(&tmpFcb
              ,(PFILE_NAMES_INFORMATION)Buffer,BufferLength);
        break;
       case FileDirectoryInformation:
        RC= FsdGetFileDirectoryInformation(&tmpFcb
              ,DeviceExt,(PFILE_DIRECTORY_INFORMATION)Buffer,BufferLength);
        break;
       case FileFullDirectoryInformation :
        RC= FsdGetFileFullDirectoryInformation(&tmpFcb
              ,DeviceExt,(PFILE_FULL_DIRECTORY_INFORMATION)Buffer,BufferLength);
        break;
       case FileBothDirectoryInformation :
        RC=FsdGetFileBothInformation(&tmpFcb
              ,DeviceExt,(PFILE_BOTH_DIRECTORY_INFORMATION)Buffer,BufferLength);
        break;
       default:
        RC=STATUS_INVALID_INFO_CLASS;
      }
    }
    else
    {
      if(Buffer0) Buffer0->NextEntryOffset=0;
      break;
    }
    if(RC==STATUS_BUFFER_OVERFLOW)
    {
      if(Buffer0) Buffer0->NextEntryOffset=0;
      pCcb->StartEntry=OldEntry;
      break;
    }
    Buffer0=(PFILE_NAMES_INFORMATION)Buffer;
    Buffer0->FileIndex=FileIndex++;
    if(Stack->Flags & SL_RETURN_SINGLE_ENTRY) break;
    BufferLength -= Buffer0->NextEntryOffset;
    Buffer += Buffer0->NextEntryOffset;
  }
  if(Buffer0) Buffer0->NextEntryOffset=0;
  if(FileIndex>0) return STATUS_SUCCESS;
  return RC;
}


NTSTATUS FsdDirectoryControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: directory control : read/write directory informations
 */
{
 NTSTATUS RC = STATUS_SUCCESS;
 PFILE_OBJECT FileObject = NULL;
 PIO_STACK_LOCATION Stack;
   Stack = IoGetCurrentIrpStackLocation(Irp);
   CHECKPOINT;
   FileObject = Stack->FileObject;
   switch (Stack->MinorFunction)
   {
    case IRP_MN_QUERY_DIRECTORY:
      RC=DoQuery(DeviceObject,Irp,Stack);
      break;
    case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
      DPRINT(" vfat, dir : change\n");
      RC=STATUS_NOT_IMPLEMENTED;
      break;
    default:
      // error
      DbgPrint("unexpected minor function %x in VFAT driver\n",Stack->MinorFunction);
	RC = STATUS_INVALID_DEVICE_REQUEST;
	break;
   }
   Irp->IoStatus.Status = RC;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return RC;
}

