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
#include <internal/string.h>
#include <wstring.h>

//#define NDEBUG
#include <internal/debug.h>

#include "ext2fs.h"

/* FUNCTIONS *****************************************************************/

BOOL Ext2ScanDir(PDEVICE_EXTENSION DeviceExt,
		 struct ext2_inode* dir, 
		 PCH filename,
		 struct ext2_dir_entry* ret)
{
   ULONG i;
   char* buffer;
   ULONG offset;
   char name[255];
   struct ext2_dir_entry* current;
   ULONG block;
   
   DPRINT("Ext2ScanDir(dir %x, filename %s, ret %x)\n",dir,filename,ret);
   
   buffer = ExAllocatePool(NonPagedPool, BLOCKSIZE);

   for (i=0; (block = Ext2BlockMap(DeviceExt, dir, i)) != 0; i++)
     {
	DPRINT("block %d\n",block);
	Ext2ReadSectors(DeviceExt->StorageDevice,
			block,
			1,
			buffer);
	
	offset = 0;
	while (offset < BLOCKSIZE)
	  {
	     current = &buffer[offset];
	     
	     strncpy(name,current->name,current->name_len);
             name[current->name_len]=0;
	     
	     DPRINT("Scanning offset %d inode %d name %s\n",
		    offset,current->inode,name);

             DPRINT("Comparing %s %s\n",name,filename);
	     if (strcmp(name,filename)==0)
	       {
		  DPRINT("Match found\n");
		  memcpy(ret,current,sizeof(struct ext2_dir_entry));
		  ExFreePool(buffer);
		  return(TRUE);
	       }
	     
	     offset = offset + current->rec_len;
	     assert(current->rec_len != 0);
	     DPRINT("offset %d\n",offset);
	  }
	DPRINT("Onto next block\n");
     }
   DPRINT("No match\n");
   ExFreePool(buffer);
   return(FALSE);
}

void unicode_to_ansi(PCH StringA, PWSTR StringW)
{
   while((*StringW)!=0)
     {
	*StringA = *StringW;
	StringA++;
	StringW++;
     }
   *StringA = 0;
}

NTSTATUS Ext2OpenFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject, 
		     PWSTR FileName)
/*
 * FUNCTION: Opens a file
 */
{
   struct ext2_inode parent_inode;
   struct ext2_dir_entry entry;
   char name[255];
   ULONG current_inode = 2;
   char* current_segment;
   PEXT2_FCB Fcb;
   
   DPRINT("Ext2OpenFile(DeviceExt %x, FileObject %x, FileName %w)\n",
	  DeviceExt,FileObject,FileName);
   
   Fcb = ExAllocatePool(NonPagedPool, sizeof(EXT2_FCB));
   
   unicode_to_ansi(name,FileName);
   DbgPrint("name %s\n",name);
   
   current_segment = strtok(name,"\\");
   do
     {
	Ext2ReadInode(DeviceExt,
		      current_inode,
		      &parent_inode);
        if (!Ext2ScanDir(DeviceExt,&parent_inode,current_segment,&entry))
	  {
	     ExFreePool(Fcb);
	     return(STATUS_UNSUCCESSFUL);
	  }
	current_inode = entry.inode;
	current_segment = strtok(NULL,"\\");
     } while(current_segment!=NULL);
   DPRINT("Found file\n");
   
   Ext2ReadInode(DeviceExt,
		 current_inode,
		 &Fcb->inode);
   FileObject->FsContext = Fcb;
   
   return(STATUS_SUCCESS);
}

NTSTATUS Ext2Create(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PFILE_OBJECT FileObject = Stack->FileObject;
   NTSTATUS Status;
   PDEVICE_EXTENSION DeviceExt;
   
   DPRINT("Ext2Create(DeviceObject %x, Irp %x)\n",DeviceObject,Irp);
   
   DeviceExt = DeviceObject->DeviceExtension;
   Status = Ext2OpenFile(DeviceExt,FileObject,FileObject->FileName.Buffer);
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(Status);
}
