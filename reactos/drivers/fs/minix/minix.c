/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/minix/minix.c
 * PURPOSE:          Minix FSD
 * PROGRAMMER:       David Welch (welch@mcmail.com)
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/string.h>
#include <wstring.h>

#define NDEBUG
#include <internal/debug.h>

#include "minix_fs.h"

/* GLOBALS ******************************************************************/

static PDRIVER_OBJECT DriverObject;

typedef struct
{
   PDEVICE_OBJECT AttachedDevice;
   struct minix_inode root_inode;
   char superblock_buf[BLOCKSIZE];
   struct minix_super_block* sb;
} MINIX_DEVICE_EXTENSION;

/* FUNCTIONS ****************************************************************/


static unsigned int MinixGetIndirectBlock(struct minix_inode* inode, 
					  unsigned char* buffer, int blk)
{
   unsigned short int* buf = (unsigned short int *)buffer;
   return(buf[blk]);
}

static unsigned int MinixGetBlock(PDEVICE_OBJECT DeviceObject,
				  struct minix_inode* inode, 
				  int blk)
{
   int block;
   char* buffer;
   
   
   DPRINT("MinixGetBlock(inode %x, blk %d)\n",inode,blk);
   
   if (blk < 7)
     {
	block = inode->i_zone[blk];
	return(block);
     }
   blk = blk - 7;
   
   buffer = ExAllocatePool(NonPagedPool,1024);
   
   if (blk < 512)
     {
	block = inode->i_zone[7];
	MinixReadSector(DeviceObject,block,buffer);
	block = MinixGetIndirectBlock(inode,buffer,blk);
	ExFreePool(buffer);
	return(block);
     }
   blk = blk - 512;
   block = inode->i_zone[8];
   MinixReadSector(DeviceObject,block,buffer);
   block = MinixGetIndirectBlock(inode,buffer,(blk>>9)&511);
   MinixReadSector(DeviceObject,block,buffer);
   block = MinixGetIndirectBlock(inode,buffer,blk&511);
   ExFreePool(buffer);
   return(block);
}

NTSTATUS MinixReadBlock(PDEVICE_OBJECT DeviceObject,
			struct minix_inode* inode, 
			int blk,
			PVOID buffer)
{
   unsigned int block;

   DPRINT("MinixReadBlock(inode %x, blk %d, buffer %x)\n",inode,blk,buffer);
   block = MinixGetBlock(DeviceObject,inode,blk);
   DPRINT("block %d\n",block);
   return(MinixReadSector(DeviceObject,block,buffer));
}

VOID MinixMount(PDEVICE_OBJECT DeviceToMount)
{
   PDEVICE_OBJECT DeviceObject;
   MINIX_DEVICE_EXTENSION* DeviceExt;
   
   IoCreateDevice(DriverObject,
		  sizeof(MINIX_DEVICE_EXTENSION),
		  NULL,
		  FILE_DEVICE_FILE_SYSTEM,
		  0,
		  FALSE,
		  &DeviceObject);
   DeviceObject->Flags = DeviceObject->Flags | DO_DIRECT_IO;
   DeviceExt = DeviceObject->DeviceExtension;
   
   MinixReadSector(DeviceToMount,1,DeviceExt->superblock_buf);
   DeviceExt->sb = (struct minix_super_block *)(DeviceExt->superblock_buf);
   
   DeviceExt->AttachedDevice = IoAttachDeviceToDeviceStack(DeviceObject,
							   DeviceToMount);
}

NTSTATUS MinixFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PVPB	vpb = Stack->Parameters.Mount.Vpb;
   PDEVICE_OBJECT DeviceToMount = Stack->Parameters.Mount.DeviceObject;
   NTSTATUS Status;
   char* superblock_buf;
   struct minix_super_block* sb;
   
   DbgPrint("MinixFileSystemControl(DeviceObject %x, Irp %x)\n",DeviceObject,
	  Irp);
   DPRINT("DeviceToMount %x\n",DeviceToMount);

   superblock_buf = ExAllocatePool(NonPagedPool,BLOCKSIZE);
   
   DPRINT("MinixReadSector %x\n",MinixReadSector);
   MinixReadSector(DeviceToMount,1,superblock_buf);
   sb = (struct minix_super_block *)superblock_buf;
   DPRINT("Magic %x\n",sb->s_magic);
   DPRINT("Imap blocks %x\n",sb->s_imap_blocks);
   DPRINT("Zmap blocks %x\n",sb->s_zmap_blocks);
   if (sb->s_magic==MINIX_SUPER_MAGIC2)
     {
	DPRINT("%s() = STATUS_SUCCESS\n",__FUNCTION__);
	MinixMount(DeviceToMount);
	Status = STATUS_SUCCESS;
     }
   else
     {
	DPRINT("%s() = STATUS_UNRECOGNIZED_VOLUME\n",__FUNCTION__);
	Status = STATUS_UNRECOGNIZED_VOLUME;
     }
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(Status);
}


NTSTATUS MinixReadInode(PDEVICE_OBJECT DeviceObject,
			MINIX_DEVICE_EXTENSION* DeviceExt,
			ULONG ino, 
			struct minix_inode* result)
{
   int block;
   char* buffer;
   struct minix_inode* inodes;
   
   DPRINT("MinixReadInode(ino %x, result %x)\n",ino,result);
   
   buffer = ExAllocatePool(NonPagedPool,1024);
   inodes = (struct minix_inode *)buffer;
   
   block = 2 + DeviceExt->sb->s_imap_blocks + DeviceExt->sb->s_zmap_blocks 
           + ((ino-1) / MINIX_INODES_PER_BLOCK);
   DPRINT("Reading block %x offset %x\n",block,block*BLOCKSIZE);
   DPRINT("Index %x\n",(ino-1)%MINIX_INODES_PER_BLOCK);
   MinixReadSector(DeviceObject,block,buffer);
   memcpy(result,&inodes[(ino-1)%MINIX_INODES_PER_BLOCK],
	  sizeof(struct minix_inode));
   DPRINT("result->i_uid %x\n",result->i_uid);
   DPRINT("result->i_size %x\n",result->i_size);
   return(STATUS_SUCCESS);
}

BOOLEAN MinixCompareUnicodeStringToAnsi(PCH AnsiStr, PWCHAR UnicodeStr,
					ULONG MaxLen)
{
   unsigned int i = 0;
   
   while (i<MaxLen)
     {
	if ((*AnsiStr)!=(*UnicodeStr))
	  {
	     return(FALSE);
	  }
	if ((*AnsiStr)==0 && (*UnicodeStr)==0)
	  {
	     return(TRUE);
	  }
	AnsiStr++;
	UnicodeStr++;
	i++;
     }
   return(TRUE);
}

ULONG MinixDirLookup(PDEVICE_OBJECT DeviceObject,
		     struct minix_inode* dir,
		     PWCHAR Name)
{
   char* buffer;
   struct minix_dir_entry* entry;
   unsigned int offset;
   unsigned int i;
   unsigned int inode;
   
   buffer=ExAllocatePool(NonPagedPool,BLOCKSIZE);
   
   for (i=0;i<(dir->i_size/MINIX_DIR_ENTRY_SIZE);i++)
     {
	offset = i*MINIX_DIR_ENTRY_SIZE;
	if ((offset%BLOCKSIZE)==0)
	  {
	     MinixReadBlock(DeviceObject,
			    dir,
			    offset/BLOCKSIZE,
			    buffer);
	  }
	entry = (struct minix_dir_entry *)&buffer[offset%BLOCKSIZE];
	DPRINT("Inode %x Name %.30s\n",entry->inode,entry->name);
        if (MinixCompareUnicodeStringToAnsi(entry->name,Name,30))
	  {
	     inode = entry->inode;
	     ExFreePool(buffer);
	     DPRINT("MinixDirLookup() = %d\n",inode);
	     return(inode);
	  }
     }
   ExFreePool(buffer);
   DPRINT("MinixDirLookup() = %d\n",0);
   return(0);
}

NTSTATUS MinixOpen(PDEVICE_OBJECT DeviceObject,
		   MINIX_DEVICE_EXTENSION* DeviceExt,
		   PWSTR DeviceName,
		   struct minix_inode* result)
{
   PWSTR current;
   PWSTR next;
   PWSTR string = DeviceName;
   struct minix_inode current_dir;
   unsigned int current_ino;
   
   DbgPrint("MinixOpen(DeviceObject %x, DeviceName %w, result %x)\n",
	  DeviceObject,DeviceName,result);
   DPRINT("DeviceName %x\n",DeviceName);
   
   next = &string[0];
   current = next+1;
   
   current_ino = MINIX_ROOT_INO;
   
   while (next!=NULL && current_ino!=0)
     {	
	MinixReadInode(DeviceObject,DeviceExt,current_ino,&current_dir);

	DPRINT("current %w next %x\n",current,next);
	
	*next = '\\';
	current = next+1;
	next = wcschr(next+1,'\\');
	if (next!=NULL)
	  {
	     *next=0;
	  }
	
        current_ino = MinixDirLookup(DeviceObject,&current_dir,current);
     }
   if (next==NULL && current_ino!=0)
     {
	MinixReadInode(DeviceObject,DeviceExt,current_ino,&current_dir);       
     }
   memcpy(result,&current_dir,sizeof(struct minix_inode));
   DPRINT("MinxOpen() = STATUS_SUCCESS\n",0);
   return(STATUS_SUCCESS);
}

NTSTATUS MinixWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   DPRINT("MinixWrite(DeviceObject %x Irp %x)\n",DeviceObject,Irp);
   
   Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
   Irp->IoStatus.Information = 0;
   return(STATUS_UNSUCCESSFUL);
}

NTSTATUS MinixRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   ULONG Length;
   PVOID Buffer;
   ULONG Offset;
   ULONG CurrentOffset;
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PFILE_OBJECT FileObject = Stack->FileObject;
   MINIX_DEVICE_EXTENSION* DeviceExt = DeviceObject->DeviceExtension;
   struct minix_inode* inode = (struct minix_inode *)FileObject->FsContext;
   PVOID TempBuffer;
   unsigned int i;
   
   DPRINT("MinixRead(DeviceObject %x, Irp %x)\n",DeviceObject,Irp);
   
   Length = Stack->Parameters.Read.Length;
   Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
   Offset = Stack->Parameters.Read.ByteOffset.LowPart;
   
   TempBuffer = ExAllocatePool(NonPagedPool,BLOCKSIZE);
   
   DPRINT("Length %x Buffer %x Offset %x\n",Length,Buffer,Offset);
   
   CurrentOffset=Offset;
   
   if ((Offset%BLOCKSIZE)!=0)
     {
	CHECKPOINT;
	
	CurrentOffset = Offset - (Offset%BLOCKSIZE);
	
	MinixReadBlock(DeviceExt->AttachedDevice,inode,
		       CurrentOffset/BLOCKSIZE,
		       TempBuffer);
	memcpy(Buffer,TempBuffer+(Offset%BLOCKSIZE),
		  min(BLOCKSIZE - (Offset%BLOCKSIZE),Length));
	DPRINT("(BLOCKSIZE - (Offset%BLOCKSIZE)) %d\n",
	       (BLOCKSIZE - (Offset%BLOCKSIZE)));
	DPRINT("Length %d\n",Length);
	CurrentOffset = CurrentOffset + BLOCKSIZE;
	Buffer = Buffer + BLOCKSIZE - (Offset%BLOCKSIZE);
	Length = Length - min(BLOCKSIZE - (Offset%BLOCKSIZE),Length);
	DPRINT("CurrentOffset %d Buffer %x Length %d\n",CurrentOffset,Buffer,
	       Length);
     }
   for (i=0;i<(Length/BLOCKSIZE);i++)
     {
	CHECKPOINT;
	
	MinixReadBlock(DeviceExt->AttachedDevice,inode,
		       CurrentOffset/BLOCKSIZE,Buffer);
	CurrentOffset = CurrentOffset + BLOCKSIZE;
	Buffer = Buffer + BLOCKSIZE;
	Length = Length - BLOCKSIZE;
     }
   if (Length > 0)
     {
	CHECKPOINT;
	
	MinixReadBlock(DeviceExt->AttachedDevice,inode,
		       CurrentOffset/BLOCKSIZE,
		       TempBuffer);	
	memcpy(Buffer,TempBuffer,Length);
     }
   
   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = Length;
   return(STATUS_SUCCESS);
}

NTSTATUS MinixClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PFILE_OBJECT FileObject = Stack->FileObject;

   DPRINT("MinixClose(DeviceObject %x Irp %x)\n",DeviceObject,Irp);
   
   ExFreePool(FileObject->FsContext);

   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(STATUS_SUCCESS);
}

NTSTATUS MinixCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PFILE_OBJECT FileObject = Stack->FileObject;
   NTSTATUS Status;
   struct minix_inode* result;
   MINIX_DEVICE_EXTENSION* DeviceExt;
   
   DPRINT("MinixCreate(DeviceObject %x, Irp %x)\n",DeviceObject,Irp);
   DPRINT("Opening file %x %w\n",FileObject->FileName.Buffer,
	    FileObject->FileName.Buffer);
   DPRINT("FileObject->FileName.Buffer %x\n",
	    FileObject->FileName.Buffer);
   
   DeviceExt = (MINIX_DEVICE_EXTENSION *)DeviceObject->DeviceExtension;
   result = ExAllocatePool(NonPagedPool,sizeof(struct minix_inode));
   DPRINT("result %x\n",result);
   Status = MinixOpen(DeviceExt->AttachedDevice,DeviceExt,
		      FileObject->FileName.Buffer,result);
   
   if (Status==STATUS_SUCCESS)
     {
	FileObject->FsContext=result;
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
   
   DbgPrint("Minix FSD 0.0.1\n");
          
   DriverObject = _DriverObject;
   
   RtlInitAnsiString(&astr,"\\Device\\Minix");
   RtlAnsiStringToUnicodeString(&ustr,&astr,TRUE);
   ret = IoCreateDevice(DriverObject,0,&ustr,
                        FILE_DEVICE_PARALLEL_PORT,0,FALSE,&DeviceObject);
   if (ret!=STATUS_SUCCESS)
     {
	return(ret);
     }

   DeviceObject->Flags=0;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = MinixClose;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = MinixCreate;
   DriverObject->MajorFunction[IRP_MJ_READ] = MinixRead;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = MinixWrite;
   DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = 
                      MinixFileSystemControl;
   DriverObject->DriverUnload = NULL;
   
   IoRegisterFileSystem(DeviceObject);
   
   return(STATUS_SUCCESS);
}

