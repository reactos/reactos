/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            mkernel/iomgr/create.cc
 * PURPOSE:         Handling file create/open apis
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  24/05/98: Created
 */

/* INCLUDES ***************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/kernel.h>
#include <internal/objmgr.h>
#include <internal/iomgr.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *************************************************************/

NTSTATUS ZwCreateFile(PHANDLE FileHandle,
		      ACCESS_MASK DesiredAccess,
		      POBJECT_ATTRIBUTES ObjectAttributes,
		      PIO_STATUS_BLOCK IoStatusBlock,
		      PLARGE_INTEGER AllocateSize,
		      ULONG FileAttributes,
		      ULONG ShareAccess,
		      ULONG CreateDisposition,
		      ULONG CreateOptions,
		      PVOID EaBuffer,
		      ULONG EaLength)
{
   UNIMPLEMENTED;
}

NTSTATUS ZwOpenFile(PHANDLE FileHandle,
		    ACCESS_MASK DesiredAccess,
		    POBJECT_ATTRIBUTES ObjectAttributes,
		    PIO_STATUS_BLOCK IoStatusBlock,
		    ULONG ShareAccess,
		    ULONG OpenOptions)
{
   UNIMPLEMENTED;
}

HANDLE STDCALL CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess,
			   DWORD dwShareMode, 
			   LPSECURITY_ATTRIBUTES lpSecurityAttributes,
			   DWORD dwCreationDisposition,
			   DWORD dwFlagsAndAttributes,
			   HANDLE hTemplateFile)
{
   PDEVICE_OBJECT DeviceObject;
   PIRP Irp;
   HANDLE hfile;
   UNICODE_STRING filename;
   ANSI_STRING afilename;
   
   RtlInitAnsiString(&afilename,lpFileName);
   RtlAnsiStringToUnicodeString(&filename,&afilename,TRUE);
   DeviceObject = ObLookupObject(NULL,&filename);   
   DPRINT("Sending IRP for IRP_MJ_CREATE to %x\n",DeviceObject);
   if (DeviceObject==NULL)
   {
      DPRINT("(%s:%d) Object not found\n",__FILE__,__LINE__);
      return(NULL);	
   }

   hfile = ObAddHandle(DeviceObject);
   
   /*
    * Tell the device we are openining it 
    */
   Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);   
   if (Irp==NULL)
     {
	return(NULL);
     }
   
   DPRINT("Preparing IRP\n");
   
   /*
    * Set up the stack location 
    */
   Irp->Stack[Irp->CurrentLocation].MajorFunction = IRP_MJ_CREATE;
   Irp->Stack[Irp->CurrentLocation].MinorFunction = 0;
   Irp->Stack[Irp->CurrentLocation].Flags = 0;
   Irp->Stack[Irp->CurrentLocation].Control = 0;
   Irp->Stack[Irp->CurrentLocation].DeviceObject = DeviceObject;
//   Irp->Stack[Irp->StackPtr].FileObject = &files[hfile];
   
   DPRINT("Sending IRP\n");
   IoCallDriver(DeviceObject,Irp);
   
   DPRINT("Returning %x\n",hfile);
   return(hfile);
}

