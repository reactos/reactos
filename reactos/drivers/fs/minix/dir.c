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
#include <string.h>

//#define NDEBUG
#include <debug.h>

#include "minix.h"

/* FUNCTIONS ****************************************************************/

BOOLEAN MinixCompareUnicodeStringToAnsi(PCH AnsiStr, 
					PWCHAR UnicodeStr,
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

#define ENTRIES_PER_BLOCK (BLOCKSIZE / MINIX_DIR_ENTRY_SIZE)

ULONG MinixDirLookup(PMINIX_DEVICE_EXTENSION DeviceExt,
		     PDEVICE_OBJECT DeviceObject,
		     struct minix_inode* dir,
		     PWCHAR Name)
{
   struct minix_dir_entry* current_entry = NULL;
   unsigned int offset;
   unsigned int i;
   unsigned int inode;
   PVOID Block;
   ULONG DiskOffset;
   
   DPRINT("MinixDirLookup(DeviceExt %x, dir %x, Name %S)\n",DeviceExt,dir,
	  Name);
   
   Block = ExAllocatePool(NonPagedPool, 512);
   
   for (i=0;i<(dir->i_size/MINIX_DIR_ENTRY_SIZE);i++)
     {
	CHECKPOINT;
	offset = i*MINIX_DIR_ENTRY_SIZE;
	if ((offset%BLOCKSIZE)==0)
	  {
	     MinixReadBlock(DeviceObject,
			    DeviceExt,
			    dir,
			    offset/BLOCKSIZE,
			    &DiskOffset);
	     MinixReadSector(DeviceObject,
			     DiskOffset,
			     Block);
	  }
	current_entry = (struct minix_dir_entry *)
	  (Block+offset%BLOCKSIZE);
	DPRINT("Inode %x Name %.30s\n",current_entry->inode,
	       current_entry->name);
        if (MinixCompareUnicodeStringToAnsi(current_entry->name,
					    Name,30))
	  {
	     inode = current_entry->inode;
	     ExFreePool(Block);
	     DPRINT("MinixDirLookup() = %d\n",inode);
	     return(inode);
	  }
     }
   CHECKPOINT;
   ExFreePool(Block);
   DPRINT("MinixDirLookup() = %d\n",0);
   return(0);
}

NTSTATUS MinixOpen(PDEVICE_OBJECT DeviceObject,
		   MINIX_DEVICE_EXTENSION* DeviceExt,
		   PFILE_OBJECT FileObject,
		   PMINIX_FSCONTEXT result,
		   PULONG Information)
{
   PWSTR current;
   PWSTR next;
   PWSTR string;
   struct minix_inode current_dir;
   unsigned int current_ino;
   
   string = ExAllocatePool(NonPagedPool, 
			   2*(wcslen(FileObject->FileName.Buffer)+1));
   wcscpy(string, FileObject->FileName.Buffer);
   
   DbgPrint("MinixOpen(DeviceObject %x, DeviceName %S, result %x)\n",
	  DeviceObject,string,result);
   
   
   next = &string[0];
   current = next+1;
   
   current_ino = MINIX_ROOT_INO;
   
   while (next!=NULL && current_ino!=0)
     {	
	MinixReadInode(DeviceObject,DeviceExt,current_ino,&current_dir);

	DPRINT("current %S next %x\n",current,next);
	
	*next = '\\';
	current = next+1;
	next = wcschr(next+1,'\\');
	if (next!=NULL)
	  {
	     *next=0;
	  }
	
        current_ino = MinixDirLookup(DeviceExt,
				     DeviceObject,
				     &current_dir,
				     current);
     }
   if (next==NULL && current_ino!=0)
     {
	MinixReadInode(DeviceObject,DeviceExt,current_ino,&current_dir);       
     }
   else
     {
	(*Information) = FILE_DOES_NOT_EXIST;
	return(STATUS_UNSUCCESSFUL);
     }
   
   result = ExAllocatePool(NonPagedPool, sizeof(MINIX_FSCONTEXT));
   memcpy(&result->inode,&current_dir,sizeof(struct minix_inode));
   
   DPRINT("MinxOpen() = STATUS_SUCCESS\n",0);
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

NTSTATUS MinixDirectoryControl(PDEVICE_OBJECT DeviceObject,
			       PIRP Irp)
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
//   PFILE_OBJECT FileObject = Stack->FileObject;
   
   if (Stack->MinorFunction != IRP_MN_QUERY_DIRECTORY)
     {
	return(STATUS_NOT_IMPLEMENTED);
     }
   
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
   PMINIX_FSCONTEXT result;
   MINIX_DEVICE_EXTENSION* DeviceExt;
   
   DPRINT("MinixCreate(DeviceObject %x, Irp %x)\n",DeviceObject,Irp);
   DPRINT("Opening file %x %S\n",FileObject->FileName.Buffer,
	    FileObject->FileName.Buffer);
   DPRINT("FileObject->FileName.Buffer %x\n",
	    FileObject->FileName.Buffer);
   
   DeviceExt = (MINIX_DEVICE_EXTENSION *)DeviceObject->DeviceExtension;
   result = ExAllocatePool(NonPagedPool,sizeof(struct minix_inode));
   DPRINT("result %x\n",result);
   Status = MinixOpen(DeviceExt->AttachedDevice,
		      DeviceExt,
		      FileObject,
		      result,
		      &Irp->IoStatus.Information);
   
   if (NT_SUCCESS(Status))
     {
	FileObject->FsContext=result;
     }
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   DPRINT("Finished MinixCreate()\n");
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(Status);
}

