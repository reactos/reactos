/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/iface.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 * UPDATE HISTORY: 
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
   
   DPRINT("FATsector %d FATeis %d\n",FATsector,FATeis);
   DPRINT("DeviceExt->FATStart %d\n",DeviceExt->FATStart);
   
   VFATReadSector(DeviceExt->StorageDevice,DeviceExt->FATStart+FATsector,
		  Block);

   DPRINT("offset %x\n",(&Block[FATeis])-Block);
   
   CurrentCluster = Block[FATeis];
   
   DPRINT("CurrentCluster %x\n",CurrentCluster);
                                
   if (CurrentCluster >= 0xfff8 && CurrentCluster <= 0xffff)
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
	
   VFATReadSector(DeviceExt->StorageDevice,DeviceExt->FATStart
		  +FATsector,CBlock);
   
   FATOffset = (CurrentCluster * 12) % (512 * 8);
   
//   DPRINT("FATSector %d FATOffset %d\n",FATsector,FATOffset);
   
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
	
//   DPRINT("Entry %x\n",Entry);
   
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
   
   DPRINT("GetNextCluster(DeviceExt %x, CurrentCluster %d)\n",
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

wchar_t * wcsncat(wchar_t * dest,const wchar_t * src,size_t count)
{
   int i,j;
   
   for (j=0;dest[j]!=0;j++);
   for (i=0;i<count;i++)
     {
	dest[j+i] = src[i];
	if (src[i] == ' ' || src[i] == 0)
	  {
	     return(dest);
	  }
     }
   dest[j+i]=0;
   return(dest);
}

wchar_t * _wcsncpy(wchar_t * dest,const wchar_t *src,size_t count)
{
   int i;
   
   for (i=0;i<count;i++)
     {
	dest[i] = src[i];
	if (src[i] == ' ' || src[i] == 0)
	  {
	     return(dest);
	  }
     }
   dest[i]=0;
   return(dest);
}


BOOLEAN IsLastEntry(PVOID Block, ULONG Offset)
{
   return(((FATDirEntry *)Block)[Offset].Filename[0] == 0);
}

BOOLEAN IsDeletedEntry(PVOID Block, ULONG Offset)
{
   return(((FATDirEntry *)Block)[Offset].Filename[0] == 0xe5);
}

BOOLEAN GetEntryName(PVOID Block, PULONG _Offset, PWSTR Name)
{
   FATDirEntry* test;
   slot* test2;
   ULONG Offset = *_Offset;
   
   test = (FATDirEntry *)Block;
   test2 = (slot *)Block;
   
   *Name = 0;

   if (IsDeletedEntry(Block,Offset))
     {
	return(FALSE);
     }
   
   DPRINT("Offset %d test2[Offset].attr %x\n",Offset,test2[Offset].attr);
   if(test2[Offset].attr == 0x0f) 
     {
	DPRINT("Parsing long name record\n");
	wcsncpy(Name,test2[Offset].name0_4,5);
	DPRINT("Name %w\n",Name);
	wcsncat(Name,test2[Offset].name5_10,6);
	DPRINT("Name %w\n",Name);
	wcsncat(Name,test2[Offset].name11_12,2);
	DPRINT("Name %w\n",Name);
	
	while((test2[Offset].id!=0x41) && (test2[Offset].id!=0x01) &&
	      (test2[Offset].attr>0)) 
	  {
	     Offset++;
	     
	     DPRINT("Reading next long name record\n");
	     
	     wcsncat(Name,test2[Offset].name0_4,5);
	     wcsncat(Name,test2[Offset].name5_10,6);
	     wcsncat(Name,test2[Offset].name11_12,2);	     
	  }
	
	if (IsDeletedEntry(Block,Offset+1))
	  {
	     Offset++;
	     *_Offset = Offset;
	     return(FALSE);
	  }
	
	*_Offset = Offset;
	
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
   ULONG i, j, k;
   ULONG Size;
   char* block;
   WCHAR name[255];
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
	VFATReadSector(DeviceExt->StorageDevice,StartingSector,block);

	
	DPRINT("%u\n", StartingSector+j);

	for (i=0; i<ENTRIES_PER_SECTOR; i++)
	  {
	     if (IsLastEntry((PVOID)block,i))
	       {
		  ExFreePool(block);
		  return(STATUS_SUCCESS);
	       }
	     if (GetEntryName((PVOID)block,&i,name))
	       {
		  DPRINT("Scanning %w\n",name);
		  
		  DPRINT("Comparing %w %w\n",name,FileToFind);
		  if (wstrcmpi(name,FileToFind))
		    {
		       DPRINT("Found match\n");
		       memcpy(&Fcb->entry,&((FATDirEntry *)block)[i],
			      sizeof(FATDirEntry));
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
   VFATReadSector(DeviceToMount, 0, (UCHAR *)Boot);
   
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
   DPRINT("<VFAT> Mounting device...");
   DPRINT("DeviceExt %x\n",DeviceExt);
   
   DeviceExt->Boot = ExAllocatePool(NonPagedPool,512);
   
   VFATReadSector(DeviceToMount, 0, (UCHAR *)DeviceExt->Boot);
   
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
}

void VFATLoadCluster(PDEVICE_EXTENSION DeviceExt, PVOID Buffer, ULONG Cluster)
{
   ULONG Sector;
   ULONG i;
   
//   DPRINT("VFATLoadCluster(DeviceExt %x, Buffer %x, Cluster %d)\n",
//	  DeviceExt,Buffer,Cluster);
   
   Sector = ClusterToSector(DeviceExt, Cluster);
   
   for (i=0; i<DeviceExt->Boot->SectorsPerCluster; i++)
     {
	VFATReadSector(DeviceExt->StorageDevice,
		       Sector+i,
		       Buffer+(i*DeviceExt->Boot->BytesPerSector));
     }
}

NTSTATUS FsdReadFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
		     PVOID Buffer, ULONG Length, ULONG ReadOffset)
/*
 * FUNCTION: Reads data from a file
 */
{
   ULONG CurrentCluster;
   ULONG FileOffset;
   ULONG i;
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
   DPRINT("FirstCluster %d\n",FirstCluster);
   DPRINT("CurrentCluster %d\n",CurrentCluster);
   
   Temp = ExAllocatePool(NonPagedPool,DeviceExt->BytesPerCluster);
   
   for (FileOffset=0; FileOffset < FirstCluster; FileOffset++)
     {
	CurrentCluster = GetNextCluster(DeviceExt,CurrentCluster);
        if (CurrentCluster == 0)
        {
                ExFreePool(Temp);
                return(STATUS_UNSUCCESSFUL);
        }
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
   ExFreePool(Temp);
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

   DPRINT("<VFAT> FsdCreate...\n");

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
   
   DPRINT("<VFAT> FSC\n");

   if (FsdHasFileSystem(DeviceToMount))
     {
	Status = FsdMount(DeviceToMount);
     }
   else
     {
        DPRINT("<VFAT> Unrecognized Volume\n");
	Status = STATUS_UNRECOGNIZED_VOLUME;
     }
   
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
   
   DbgPrint("VFAT 0.0.1\n");
          
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

