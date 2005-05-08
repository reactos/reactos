/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/ext2/super.c
 * PURPOSE:          ext2 filesystem
 * PROGRAMMER:       David Welch (welch@mcmail.com)
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <string.h>

#define NDEBUG
#include <debug.h>

#include "ext2fs.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS Ext2ReadPage(PDEVICE_EXTENSION DeviceExt,
		      PEXT2_FCB Fcb,
		      PVOID Buffer,
		      ULONG Offset)
{
   ULONG block, i;
   
   for (i=0; i<4; i++)
     {
	block = Ext2BlockMap(DeviceExt, 
			     Fcb->i.inode, 
			     Offset + i);
	Ext2ReadSectors(DeviceExt->StorageDevice,
			block,
			1,
			Buffer + (i*BLOCKSIZE));
     }
   return(STATUS_SUCCESS);
}

NTSTATUS Ext2ReadFile(PDEVICE_EXTENSION DeviceExt, 
		      PFILE_OBJECT FileObject,
		      PVOID Buffer, 
		      ULONG Length, 
		      LARGE_INTEGER OffsetL)
{
   PVOID BaseAddress;
   BOOLEAN Uptodate = FALSE;
   PCACHE_SEGMENT CacheSeg;
   ULONG Offset = (ULONG)OffsetL.u.LowPart;
   PEXT2_FCB Fcb;
   ULONG block, i, Delta;
   DPRINT("Ext2ReadFile(DeviceExt %x, FileObject %x, Buffer %x, Length %d, \n"
	  "OffsetL %d)\n",DeviceExt,FileObject,Buffer,Length,(ULONG)OffsetL);

   Fcb = (PEXT2_FCB)FileObject->FsContext;

   Ext2LoadInode(DeviceExt,
		 Fcb->inode,
		 &Fcb->i);
   
   if (Offset >= Fcb->i.inode->i_size)
     {
	DPRINT("Returning end of file\n");
	return(STATUS_END_OF_FILE);
     }
   if ((Offset + Length) > Fcb->i.inode->i_size)
     {
	Length = Fcb->i.inode->i_size - Offset;
     }
   
   Ext2ReleaseInode(DeviceExt,
		    &Fcb->i);
   
   if ((Offset % PAGE_SIZE) != 0)
     {
	Delta = min(PAGE_SIZE - (Offset % PAGE_SIZE),Length);
	CcRequestCachePage(Fcb->Bcb,
			   Offset,
			   &BaseAddress,
			   &Uptodate,
			   &CacheSeg);
	if (Uptodate == FALSE)
	  {
	     Ext2ReadPage(DeviceExt,
			  Fcb,
			  BaseAddress,
			  Offset / BLOCKSIZE);
	  }
	memcpy(Buffer, BaseAddress + (Offset % PAGE_SIZE), Delta);
	CcReleaseCachePage(Fcb->Bcb,
			   CacheSeg,
			   TRUE);
	Length = Length - Delta;
	Offset = Offset + Delta;
	Buffer = Buffer + Delta;
     }
   CHECKPOINT;
   for (i=0; i<(Length/PAGE_SIZE); i++)
     {
	CcRequestCachePage(Fcb->Bcb,
			   Offset,
			   &BaseAddress,
			   &Uptodate,
			   &CacheSeg);
	if (Uptodate == FALSE)
	  {
	     Ext2ReadPage(DeviceExt,
			  Fcb,
			  BaseAddress,
			  (Offset / BLOCKSIZE));
	  }
	memcpy(Buffer, BaseAddress, PAGE_SIZE);
	CcReleaseCachePage(Fcb->Bcb,
			   CacheSeg,
			   TRUE);	
	Length = Length - PAGE_SIZE;
	Offset = Offset + PAGE_SIZE;
	Buffer = Buffer + PAGE_SIZE;
     }
   CHECKPOINT;
   if ((Length % PAGE_SIZE) != 0)
     {
	CcRequestCachePage(Fcb->Bcb,
			   Offset,
			   &BaseAddress,
			   &Uptodate,
			   &CacheSeg);
	if (Uptodate == FALSE)
	  {
	     Ext2ReadPage(DeviceExt,
			  Fcb,
			  BaseAddress,
			  (Offset / BLOCKSIZE));
	  }
	DPRINT("Copying %x to %x Length %d\n",BaseAddress,Buffer,Length);
	memcpy(Buffer,BaseAddress,Length);
	CcReleaseCachePage(Fcb->Bcb,
			   CacheSeg,
			   TRUE);	
     }
   CHECKPOINT;
   
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
Ext2Write(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   DPRINT("Ext2Write(DeviceObject %x Irp %x)\n",DeviceObject,Irp);
   
   Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
   Irp->IoStatus.Information = 0;
   return(STATUS_UNSUCCESSFUL);
}

NTSTATUS STDCALL
Ext2FlushBuffers(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   DPRINT("Ext2FlushBuffers(DeviceObject %x Irp %x)\n",DeviceObject,Irp);
   
   Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
   Irp->IoStatus.Information = 0;
   return(STATUS_UNSUCCESSFUL);
}

NTSTATUS STDCALL
Ext2Shutdown(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   DPRINT("Ext2Shutdown(DeviceObject %x Irp %x)\n",DeviceObject,Irp);
   
   Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
   Irp->IoStatus.Information = 0;
   return(STATUS_UNSUCCESSFUL);
}

NTSTATUS STDCALL
Ext2Cleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   DbgPrint("Ext2Cleanup(DeviceObject %x Irp %x)\n",DeviceObject,Irp);
   
   Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
   Irp->IoStatus.Information = 0;
   
   DbgPrint("Ext2Cleanup() finished\n");
   
   return(STATUS_UNSUCCESSFUL);
}

NTSTATUS STDCALL
Ext2Read(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   ULONG Length;
   PVOID Buffer;
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PFILE_OBJECT FileObject = Stack->FileObject;
   PDEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
   NTSTATUS Status;
   
   DPRINT("Ext2Read(DeviceObject %x, FileObject %x, Irp %x)\n",
	  DeviceObject, FileObject, Irp);
   
   Length = Stack->Parameters.Read.Length;
   CHECKPOINT;
   Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
   CHECKPOINT;
   CHECKPOINT;
   
   Status = Ext2ReadFile(DeviceExt,FileObject,Buffer,Length,
			 Stack->Parameters.Read.ByteOffset);
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = Length;
   IoCompleteRequest(Irp,IO_NO_INCREMENT);
   
   return(Status);
}
