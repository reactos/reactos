/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/ext2/dir.c
 * PURPOSE:          ext2 filesystem
 * PROGRAMMER:       David Welch (welch@cwcom.net)
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>

#include "ext2fs.h"

/* FUNCTIONS *****************************************************************/

VOID Ext2ConvertName(PWSTR Out, PCH In, ULONG Len)
{
   ULONG i;
   
   for (i=0; i<Len; i++)
     {
	*Out = *In;
	Out++;
	In++;
     }
   *Out = 0;
}

PVOID Ext2ProcessDirEntry(PDEVICE_EXTENSION DeviceExt,
			  struct ext2_dir_entry* dir_entry,
			  PIO_STACK_LOCATION IoStack,
			  PVOID Buffer,
			  ULONG FileIndex)
{
   PFILE_DIRECTORY_INFORMATION FDI;
   PFILE_NAMES_INFORMATION FNI;
   PFILE_BOTH_DIRECTORY_INFORMATION FBI;
   ULONG i;
   PWSTR FileName;
   struct ext2_inode inode;
   
   DPRINT("FileIndex %d\n",FileIndex);
   DPRINT("Buffer %x\n",Buffer);
   
   Ext2ReadInode(DeviceExt,
		 dir_entry->inode,
		 &inode);
     
   switch (IoStack->Parameters.QueryDirectory.FileInformationClass)
     {
      case FileNamesInformation:
	FNI = (PFILE_NAMES_INFORMATION)Buffer;
	FNI->NextEntryOffset = sizeof(FileDirectoryInformation) +
	  dir_entry->name_len + 1;
	FNI->FileNameLength = dir_entry->name_len;	
	Ext2ConvertName(FNI->FileName, dir_entry->name, dir_entry->name_len);
	Buffer = Buffer + FNI->NextEntryOffset;
	break;
	
      case FileDirectoryInformation:
	FDI = (PFILE_DIRECTORY_INFORMATION)Buffer;
	FDI->NextEntryOffset = sizeof(FileDirectoryInformation) + 
	                       dir_entry->name_len + 1;
	FDI->FileIndex = FileIndex;
//	FDI->CreationTime = 0;
//	FDI->LastAccessTime = 0;
//	FDI->LastWriteTime = 0;
//	FDI->ChangeTime = 0;
        FDI->AllocationSize.QuadPart = FDI->EndOfFile.QuadPart = inode.i_size;
	FDI->FileAttributes = 0;
	FDI->FileNameLength = dir_entry->name_len;
	Ext2ConvertName(FDI->FileName, dir_entry->name, dir_entry->name_len);
	Buffer = Buffer + FDI->NextEntryOffset;
	break;
	
      case FileBothDirectoryInformation:
	FBI = (PFILE_BOTH_DIRECTORY_INFORMATION)Buffer;
	FBI->NextEntryOffset = sizeof(FileBothDirectoryInformation) +
	  dir_entry->name_len + 1;
	FBI->FileIndex = FileIndex;
        FBI->AllocationSize.QuadPart = FBI->EndOfFile.QuadPart = inode.i_size;
	FBI->FileAttributes = 0;
	FBI->FileNameLength = dir_entry->name_len;
	Ext2ConvertName(FBI->FileName, dir_entry->name, dir_entry->name_len);
	memset(FBI->ShortName, 0, sizeof(FBI->ShortName));
	Buffer = Buffer + FBI->NextEntryOffset;
	break;
	
      default:
	UNIMPLEMENTED;
     }
   return(Buffer);
}
			  

NTSTATUS Ext2QueryDirectory(PDEVICE_EXTENSION DeviceExt,
			    PEXT2_FCB Fcb,
			    PIRP Irp,
			    PIO_STACK_LOCATION IoStack)
{
   ULONG Max;
   ULONG i;
   ULONG StartIndex;
   PVOID Buffer;
   struct ext2_dir_entry dir_entry;
   ULONG CurrentIndex;
   
   DPRINT("Buffer %x\n",Buffer);
   
   Buffer = Irp->UserBuffer;
   DPRINT("IoStack->Flags %x\n",IoStack->Flags);
   
   if (IoStack->Flags & SL_RETURN_SINGLE_ENTRY)
     {
	Max = 1;
     }
   else
     {
	UNIMPLEMENTED;
     }
   
   DPRINT("Buffer->FileIndex %d\n",
	  ((PFILE_DIRECTORY_INFORMATION)Buffer)->FileIndex);
   if (IoStack->Flags & SL_INDEX_SPECIFIED)
     {
	StartIndex = ((PFILE_DIRECTORY_INFORMATION)Buffer)->FileIndex;
     }
   else
     {
	StartIndex = 0;
     }
   
   if (IoStack->Flags & SL_RESTART_SCAN)
     {
	StartIndex = 0;
     }
   
   DPRINT("StartIndex %d\n",StartIndex);
   
   for (i=0; i<Max ;i++)
     {
	if (!Ext2ScanDir(DeviceExt,&Fcb->inode,"*",&dir_entry,&StartIndex))
	  {
	     ((PFILE_DIRECTORY_INFORMATION)Buffer)->NextEntryOffset = 0;
	     return(STATUS_NO_MORE_FILES);
	  }
	Buffer = Ext2ProcessDirEntry(DeviceExt,
				     &dir_entry, 
				     IoStack, 
				     Buffer, 
				     StartIndex);
     }
   return(STATUS_SUCCESS);
}

NTSTATUS Ext2DirectoryControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PFILE_OBJECT FileObject = Stack->FileObject;
   PEXT2_FCB Fcb = (PVOID)FileObject->FsContext;
   NTSTATUS Status;
   PDEVICE_EXTENSION DeviceExt;
   
   DPRINT("Ext2DirectoryControl(DeviceObject %x, Irp %x)\n",DeviceObject,Irp);
   
   DeviceExt = DeviceObject->DeviceExtension;
   
   switch (Stack->MinorFunction)
     {
      case IRP_MN_QUERY_DIRECTORY:
	Status = Ext2QueryDirectory(DeviceExt, Fcb, Irp, Stack);
	break;
	
      default:
	Status = STATUS_UNSUCCESSFUL;
     }
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(Status);   
}

BOOL Ext2ScanDir(PDEVICE_EXTENSION DeviceExt,
		 struct ext2_inode* dir, 
		 PCH filename,
		 struct ext2_dir_entry* ret,
		 PULONG StartIndex)
{
   ULONG i;
   char* buffer;
   ULONG offset;
   char name[255];
   struct ext2_dir_entry* current;
   ULONG block;
   
   DPRINT("Ext2ScanDir(dir %x, filename %s, ret %x)\n",dir,filename,ret);
   
   buffer = ExAllocatePool(NonPagedPool, BLOCKSIZE);
   
   for (i=0; i<((*StartIndex)/BLOCKSIZE); i++);
   for (; (block = Ext2BlockMap(DeviceExt, dir, i)) != 0; i++)
     {
	DPRINT("block %d\n",block);
	Ext2ReadSectors(DeviceExt->StorageDevice,
			block,
			1,
			buffer);
	
	offset = (*StartIndex)%BLOCKSIZE;
	while (offset < BLOCKSIZE)
	  {
	     current = &buffer[offset];
	     
	     strncpy(name,current->name,current->name_len);
             name[current->name_len]=0;
	     
	     DPRINT("Scanning offset %d inode %d name %s\n",
		    offset,current->inode,name);

             DPRINT("Comparing %s %s\n",name,filename);
	     if (strcmp(name,filename)==0 || strcmp(filename,"*")==0)
	       {
		  DPRINT("Match found\n");
		  *StartIndex = (i*BLOCKSIZE) + offset + current->rec_len;
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
   ULONG StartIndex = 0;
   
   DPRINT("Ext2OpenFile(DeviceExt %x, FileObject %x, FileName %w)\n",
	  DeviceExt,FileObject,FileName);
   
   Fcb = ExAllocatePool(NonPagedPool, sizeof(EXT2_FCB));
   
   unicode_to_ansi(name,FileName);
   DPRINT("name %s\n",name);
   
   current_segment = strtok(name,"\\");
   while (current_segment!=NULL)
     {
	Ext2ReadInode(DeviceExt,
		      current_inode,
		      &parent_inode);
        if (!Ext2ScanDir(DeviceExt,&parent_inode,current_segment,&entry,
			 &StartIndex))
	  {
	     ExFreePool(Fcb);
	     return(STATUS_UNSUCCESSFUL);
	  }
	current_inode = entry.inode;
	current_segment = strtok(NULL,"\\");
	StartIndex = 0;
     }
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
