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

#define NDEBUG
#include <internal/debug.h>

#include "minix.h"

/* FUNCTIONS ****************************************************************/

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

#define ENTRIES_PER_BLOCK (BLOCKSIZE / MINIX_DIR_ENTRY_SIZE)

ULONG MinixDirLookup(PMINIX_DEVICE_EXTENSION DeviceExt,
		     struct minix_inode* dir,
		     PWCHAR Name)
{
   struct minix_dir_entry* current_entry = NULL;
   unsigned int offset;
   unsigned int i;
   unsigned int inode;
   PCCB Ccb = NULL;
   
   DPRINT("MinixDirLookup(DeviceExt %x, dir %x, Name %w)\n",DeviceExt,dir,
	  Name);
   
   for (i=0;i<(dir->i_size/MINIX_DIR_ENTRY_SIZE);i++)
     {
	CHECKPOINT;
	offset = i*MINIX_DIR_ENTRY_SIZE;
	if ((offset%BLOCKSIZE)==0)
	  {
	     CHECKPOINT;
	     if (Ccb != NULL)
	       {
		  CHECKPOINT;
		  CbReleaseFromRead(&DeviceExt->Dccb,Ccb);
	       }
	     CHECKPOINT;
	     MinixReadBlock(DeviceExt,
			    dir,
			    offset/BLOCKSIZE,
			    &Ccb);
	  }
	current_entry = (struct minix_dir_entry *)
	  (Ccb->Buffer+offset%BLOCKSIZE);
	DPRINT("Inode %x Name %.30s\n",current_entry->inode,
	       current_entry->name);
        if (MinixCompareUnicodeStringToAnsi(current_entry->name,
					    Name,30))
	  {
	     inode = current_entry->inode;
	     CbReleaseFromRead(&DeviceExt->Dccb,Ccb);
	     DPRINT("MinixDirLookup() = %d\n",inode);
	     return(inode);
	  }
     }
   CHECKPOINT;
   if (Ccb != NULL)
     {
	CbReleaseFromRead(&DeviceExt->Dccb,Ccb);
     }
   DPRINT("MinixDirLookup() = %d\n",0);
   return(0);
}

NTSTATUS MinixOpen(PDEVICE_OBJECT DeviceObject,
		   MINIX_DEVICE_EXTENSION* DeviceExt,
		   PWSTR DeviceName,
		   struct minix_inode* result,
		   PULONG Information)
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
	
        current_ino = MinixDirLookup(DeviceExt,&current_dir,current);
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
   memcpy(result,&current_dir,sizeof(struct minix_inode));
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
   PFILE_OBJECT FileObject = Stack->FileObject;
   
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
		      FileObject->FileName.Buffer,result,
		      &Irp->IoStatus.Information);
   
   if (Status==STATUS_SUCCESS)
     {
	FileObject->FsContext=result;
     }
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   DPRINT("Finished MinixCreate()\n");
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(Status);
}

