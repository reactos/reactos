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

*/

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/string.h>
#include <wstring.h>

#define NDEBUG
#include <internal/debug.h>

#include "vfat.h"

#define FAT16 (1)
#define FAT12 (2)

typedef struct
{
   PDEVICE_OBJECT StorageDevice;
   BootSector *Boot;
   int rootDirectorySectors, FATStart, rootStart, dataStart;
   int FATEntriesPerSector, FATUnit;
   ULONG BytesPerCluster;
   ULONG FatType;
   unsigned char* FAT;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct
{
   FATDirEntry entry;
} FCB, *PFCB;

#define ENTRIES_PER_SECTOR (BLOCKSIZE / sizeof(FATDirEntry))

/* GLOBALS *****************************************************************/

static PDRIVER_OBJECT DriverObject;

/* FUNCTIONS ****************************************************************/

ULONG Fat16GetNextCluster(PDEVICE_EXTENSION DeviceExt, ULONG CurrentCluster)
{
   ULONG FATsector;
   ULONG FATeis;
   PUSHORT Block;

   Block = ExAllocatePool(NonPagedPool,1024);
   
   FATsector=CurrentCluster/(512/sizeof(USHORT));
   FATeis=CurrentCluster-(FATsector*256);
   
   
//   VFATReadSector(DeviceExt->StorageDevice,DeviceExt->FATStart+FATsector,
//		  Block);

   memcpy(Block,&DeviceExt->FAT, BLOCKSIZE);
   
   CurrentCluster = Block[FATeis];
   
   if (CurrentCluster >= 0xfff8 || CurrentCluster <= 0xffff)
     {
	CurrentCluster = 0;
     }	
   
   ExFreePool(Block);

   DPRINT("Returning %x\n",CurrentCluster);
   
   return(CurrentCluster);
}

ULONG Fat12GetNextCluster(PDEVICE_EXTENSION DeviceExt, ULONG CurrentCluster)
{
   unsigned char* CBlock;
   ULONG FATsector;
   ULONG FATOffset;
   ULONG Entry;

   CBlock = ExAllocatePool(NonPagedPool,1024);
   
   FATsector = (CurrentCluster * 12) / (512 * 8);
	
//   VFATReadSector(DeviceExt->StorageDevice,DeviceExt->FATStart
//		  +FATsector,CBlock);

   memcpy(CBlock,&DeviceExt->FAT, BLOCKSIZE);
   
   FATOffset = (CurrentCluster * 12) % (512 * 8);
   
   DPRINT("FATSector %d FATOffset %d\n",FATsector,FATOffset);
   
   if ((CurrentCluster % 2) == 0)
     {
	Entry = CBlock[((FATOffset / 24)*3)];
	Entry |= (CBlock[((FATOffset / 24)*3) + 1] & 0xf);
     }
   else
     {
	Entry = (CBlock[((FATOffset / 24)*3) + 1] >> 4);
	Entry |= (CBlock[((FATOffset / 24)*3) + 2] << 4);
     }
	
   DPRINT("Entry %x\n",Entry);
   
   if (Entry >= 0xff8 && Entry <= 0xfff)
     {
	Entry = 0;
     }
   
   CurrentCluster = Entry;
   
   ExFreePool(CBlock);

   DPRINT("Returning %x\n",CurrentCluster);
   
   return(CurrentCluster);
}

ULONG GetNextCluster(PDEVICE_EXTENSION DeviceExt, ULONG CurrentCluster)
{
   
   DPRINT("GetNextCluster(DeviceExt %x, CurrentCluster %x)\n",
	    DeviceExt,CurrentCluster);
   if (DeviceExt->FatType == FAT16)
     {
	return(Fat16GetNextCluster(DeviceExt, CurrentCluster));
     }
   else
     {	
	return(Fat12GetNextCluster(DeviceExt, CurrentCluster));
     }
}

unsigned long ClusterToSector(PDEVICE_EXTENSION DeviceExt, 
			      unsigned long Cluster)
{
  return DeviceExt->dataStart+((Cluster-2)*DeviceExt->Boot->SectorsPerCluster);
}

void RtlAnsiToUnicode(PWSTR Dest, PCH Source, ULONG Length)
{
   int i;
   
   for (i=0; (i<Length && Source[i] != ' '); i++)
     {
	Dest[i] = Source[i];
     }
   Dest[i]=0;
}

void RtlCatAnsiToUnicode(PWSTR Dest, PCH Source, ULONG Length)
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
{
   int i;
   
   for (i=0;i<wcount;i++)
     {
	*dest=src[i];
	dest++;
     }
   dest=dest-wcount;

   return(dest);
}

wchar_t * vfat_movstr(wchar_t * dest, const wchar_t *src, ULONG dpos,
                      ULONG spos, ULONG len)
{
  int i;

  dest+=dpos;
  for(i=spos; i<spos+len; i++)
  {
    *dest=src[i];
    dest++;
  }
  dest-=(dpos+len);

  return(dest);
}

BOOLEAN IsLastEntry(PVOID Block, ULONG Offset)
{
   return(((FATDirEntry *)Block)[Offset].Filename[0] == 0);
}

BOOLEAN IsDeletedEntry(PVOID Block, ULONG Offset)
{
   /* Checks special character (short names) or attrib=0 (long names) */

   return ((((FATDirEntry *)Block)[Offset].Filename[0] == 0xe5) ||
           (((FATDirEntry *)Block)[Offset].Attrib == 0x00));
}

BOOLEAN GetEntryName(PVOID Block, PULONG _Offset, PWSTR Name, PULONG _jloop,
  PDEVICE_EXTENSION DeviceExt, PULONG _StartingSector)
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
               StartingSector++;
               jloop++;
               VFATReadSectors(DeviceExt->StorageDevice,StartingSector,1,Block);
               test2 = (slot *)Block;
             }
             cpos++;

             vfat_initstr(tmp, 256);
             vfat_movstr(tmp, Name, 13, 0, cpos*13);
             vfat_wcsncpy(Name, tmp, 256);
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
{
   DPRINT("s1 '%w' s2 '%w'\n",s1,s2);
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

NTSTATUS FindFile(PDEVICE_EXTENSION DeviceExt, PFCB Fcb,
		  PFCB Parent, PWSTR FileToFind)
{
   ULONG i, j;
   ULONG Size;
   char* block;
   WCHAR name[256];
   ULONG StartingSector;
   ULONG NextCluster;
   
   DPRINT("FileFile(Parent %x, FileToFind %w)\n",Parent,FileToFind);
   
   if (Parent == NULL)
     {
	Size = DeviceExt->rootDirectorySectors;
	StartingSector = DeviceExt->rootStart;
     }
   else
     {
	DPRINT("Parent->entry.FileSize %x\n",Parent->entry.FileSize);
	
	Size = ULONG_MAX;
	StartingSector = ClusterToSector(DeviceExt, Parent->entry.FirstCluster);
	NextCluster = Parent->entry.FirstCluster;
     }
   
   block = ExAllocatePool(NonPagedPool,BLOCKSIZE);

   for (j=0; j<Size; j++)
     {
	VFATReadSectors(DeviceExt->StorageDevice,StartingSector,1,block);

	for (i=0; i<ENTRIES_PER_SECTOR; i++)
	  {
	     if (IsLastEntry((PVOID)block,i))
	       {
		  ExFreePool(block);
		  return(STATUS_UNSUCCESSFUL);
	       }
	     if (GetEntryName((PVOID)block,&i,name,&j,DeviceExt,&StartingSector))
	       {
		  DPRINT("Scanning %w\n",name);
		  DPRINT("Comparing %w %w\n",name,FileToFind);
		  if (wstrcmpi(name,FileToFind))
		    {

                       /* In the case of a long filename, the firstcluster is stored in
                          the next record -- where it's short name is */
                       if(((FATDirEntry *)block)[i].FirstCluster==0) i++;

                       DPRINT("Found it at cluster %u\n", ((FATDirEntry *)block)[i].FirstCluster);

		       memcpy(&Fcb->entry,&((FATDirEntry *)block)[i],
			      sizeof(FATDirEntry));

       		       ExFreePool(block);
		       return(STATUS_SUCCESS);
		    }
	       }

	  }
	if (Parent == NULL)
	  {
	     StartingSector++;
	  }
	else
	  {
	     NextCluster = GetNextCluster(DeviceExt,NextCluster);
	     if (NextCluster == 0)
	       {
                  ExFreePool(block);
		  return(STATUS_UNSUCCESSFUL);
	       }
	     StartingSector = ClusterToSector(DeviceExt,NextCluster);
	  }	    	     
     }
   ExFreePool(block);
   return(STATUS_UNSUCCESSFUL);
}


NTSTATUS FsdCloseFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject)
/*
 * FUNCTION: Closes a file
 */
{
   /* NOP */
}

NTSTATUS FsdOpenFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject, 
		     PWSTR FileName)
/*
 * FUNCTION: Opens a file
 */
{
   PWSTR current;
   PWSTR next;
   PWSTR string = FileName;
   PFCB ParentFcb = NULL;
   PFCB Fcb = ExAllocatePool(NonPagedPool,sizeof(FCB));
   PFCB Temp;
   NTSTATUS Status;
   
   next = &string[0];
   current = next+1;
   
   while (next!=NULL)
     {	
	DPRINT("current %w next %x\n",current,next);
	
	*next = '\\';
	current = next+1;
	next = wcschr(next+1,'\\');
	if (next!=NULL)
	  {
	     *next=0;
	  }
	
        Status = FindFile(DeviceExt,Fcb,ParentFcb,current);
	if (Status != STATUS_SUCCESS)
	  {
	     return(Status);
	  }
	Temp = Fcb;
	if (ParentFcb == NULL)
	  {
	     Fcb = ExAllocatePool(NonPagedPool,sizeof(FCB));
	     ParentFcb = Temp;
	  }
	else
	  {
	     Fcb = ParentFcb;
	     ParentFcb = Temp;
	  }
     }
   FileObject->FsContext = ParentFcb;
   DPRINT("ParentFcb->entry.FileSize %d\n",ParentFcb->entry.FileSize);

   return(STATUS_SUCCESS);
}

BOOLEAN FsdHasFileSystem(PDEVICE_OBJECT DeviceToMount)
/*
 * FUNCTION: Tests if the device contains a filesystem that can be mounted 
 * by this fsd
 */
{
   BootSector* Boot;
   
   Boot = ExAllocatePool(NonPagedPool,512);

   VFATReadSectors(DeviceToMount, 0, 1, (UCHAR *)Boot);

   if (strncmp(Boot->SysType,"FAT12",5)==0 ||
       strncmp(Boot->SysType,"FAT16",5)==0)
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
   int i;

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
   else
     {
	DeviceExt->FatType = FAT16;
     }

   DeviceExt->FAT = ExAllocatePool(NonPagedPool, BLOCKSIZE*DeviceExt->Boot->FATSectors);
   VFATReadSectors(DeviceToMount, DeviceExt->FATStart, DeviceExt->Boot->FATSectors, (UCHAR *)DeviceExt->FAT);
}

void VFATLoadCluster(PDEVICE_EXTENSION DeviceExt, PVOID Buffer, ULONG Cluster)
{
   ULONG Sector;
   ULONG i;
   
//   DPRINT("VFATLoadCluster(DeviceExt %x, Buffer %x, Cluster %d)\n",
//	  DeviceExt,Buffer,Cluster);
   
  Sector = ClusterToSector(DeviceExt, Cluster);
   
  VFATReadSectors(DeviceExt->StorageDevice,
 	          Sector,
                  DeviceExt->Boot->SectorsPerCluster,
	          Buffer);
}

NTSTATUS FsdReadFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
		     PVOID Buffer, ULONG Length, ULONG ReadOffset)
/*
 * FUNCTION: Reads data from a file
 */
{
   ULONG CurrentCluster;
   ULONG FileOffset;
   ULONG FirstCluster;
   PFCB Fcb;
   PVOID Temp;
   ULONG TempLength;
   
   DPRINT("FsdReadFile(DeviceExt %x, FileObject %x, Buffer %x, "
	    "Length %d, ReadOffset %d)\n",DeviceExt,FileObject,Buffer,
	    Length,ReadOffset);
   
   FirstCluster = ReadOffset / DeviceExt->BytesPerCluster;
   Fcb = FileObject->FsContext;
   CurrentCluster = Fcb->entry.FirstCluster;
   
   DPRINT("DeviceExt->BytesPerCluster %x\n",DeviceExt->BytesPerCluster);
   
   Temp = ExAllocatePool(NonPagedPool,DeviceExt->BytesPerCluster);
   
   for (FileOffset=0; FileOffset < FirstCluster; FileOffset++)
     {
	CurrentCluster = GetNextCluster(DeviceExt,CurrentCluster);
     }
   CHECKPOINT;
   if ((ReadOffset % DeviceExt->BytesPerCluster)!=0)
     {
	VFATLoadCluster(DeviceExt,Temp,CurrentCluster);
	CurrentCluster = GetNextCluster(DeviceExt, CurrentCluster);
	
	TempLength = min(Length,DeviceExt->BytesPerCluster -
			 (ReadOffset % DeviceExt->BytesPerCluster));
	
	memcpy(Buffer, Temp + ReadOffset % DeviceExt->BytesPerCluster,
	       TempLength);
	
	Length = Length - TempLength;
	Buffer = Buffer + TempLength;	     
     }
   CHECKPOINT;
   while (Length > DeviceExt->BytesPerCluster)
     {
	VFATLoadCluster(DeviceExt, Buffer, CurrentCluster);
	CurrentCluster = GetNextCluster(DeviceExt, CurrentCluster);
	
	if (CurrentCluster == 0)
	  {
	     return(STATUS_SUCCESS);
	  }
	
	Buffer = Buffer + DeviceExt->BytesPerCluster;
	Length = Length - DeviceExt->BytesPerCluster;
     }
   CHECKPOINT;
   if (Length > 0)
     {
	VFATLoadCluster(DeviceExt, Temp, CurrentCluster);
	memcpy(Buffer, Temp, Length);
     }
   return(STATUS_SUCCESS);
}


NTSTATUS FsdClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
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
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PFILE_OBJECT FileObject = Stack->FileObject;
   NTSTATUS Status;
   PDEVICE_EXTENSION DeviceExt;

   DPRINT("VFAT FsdCreate...\n");

   DeviceExt = DeviceObject->DeviceExtension;
   Status = FsdOpenFile(DeviceExt,FileObject,FileObject->FileName.Buffer);
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(Status);
}


NTSTATUS FsdWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   DPRINT("FsdWrite(DeviceObject %x Irp %x)\n",DeviceObject,Irp);
   
   Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
   Irp->IoStatus.Information = 0;
   return(STATUS_UNSUCCESSFUL);
}

NTSTATUS FsdRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   ULONG Length;
   PVOID Buffer;
   ULONG Offset;
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PFILE_OBJECT FileObject = Stack->FileObject;
   PDEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
   NTSTATUS Status;
   
   DPRINT("FsdRead(DeviceObject %x, Irp %x)\n",DeviceObject,Irp);
   
   Length = Stack->Parameters.Read.Length;
   Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
   Offset = Stack->Parameters.Read.ByteOffset.LowPart;
   
   Status = FsdReadFile(DeviceExt,FileObject,Buffer,Length,Offset);
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = Length;
   IoCompleteRequest(Irp,IO_NO_INCREMENT);

   return(Status);
}


NTSTATUS FsdMount(PDEVICE_OBJECT DeviceToMount)
{
   PDEVICE_OBJECT DeviceObject;
   PDEVICE_EXTENSION DeviceExt;
      
   IoCreateDevice(DriverObject,
		  sizeof(DEVICE_EXTENSION),
		  NULL,
		  FILE_DEVICE_FILE_SYSTEM,
		  0,
		  FALSE,
		  &DeviceObject);
   DeviceObject->Flags = DeviceObject->Flags | DO_DIRECT_IO;
   DeviceExt = (PVOID)DeviceObject->DeviceExtension;
   
   FsdMountDevice(DeviceExt,DeviceToMount);
   
   DeviceExt->StorageDevice = IoAttachDeviceToDeviceStack(DeviceObject,
							  DeviceToMount);
   return(STATUS_SUCCESS);
}

NTSTATUS FsdFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PVPB	vpb = Stack->Parameters.Mount.Vpb;
   PDEVICE_OBJECT DeviceToMount = Stack->Parameters.Mount.DeviceObject;
   NTSTATUS Status;
   
   DPRINT("VFAT FSC\n");

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
   
   DbgPrint("VFAT 0.0.3\n");
          
   DriverObject = _DriverObject;
   
   RtlInitAnsiString(&astr,"\\Device\\VFAT");
   RtlAnsiStringToUnicodeString(&ustr,&astr,TRUE);
   ret = IoCreateDevice(DriverObject,0,&ustr,
                        FILE_DEVICE_FILE_SYSTEM,0,FALSE,&DeviceObject);
   if (ret!=STATUS_SUCCESS)
     {
	return(ret);
     }

   DeviceObject->Flags=0;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = FsdClose;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = FsdCreate;
   DriverObject->MajorFunction[IRP_MJ_READ] = FsdRead;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = FsdWrite;
   DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
                      FsdFileSystemControl;
   DriverObject->DriverUnload = NULL;
   
   IoRegisterFileSystem(DeviceObject);

   return(STATUS_SUCCESS);
}

